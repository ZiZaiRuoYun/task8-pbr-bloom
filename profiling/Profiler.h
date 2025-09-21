#pragma once
#include <glad/glad.h>
#include <chrono>
#include <unordered_map>
#include <string>
#include <cstdio>

struct CpuTimer {
    using clock = std::chrono::high_resolution_clock;
    clock::time_point t0;
    void begin() { t0 = clock::now(); }
    double end_ms() const {
        return std::chrono::duration<double, std::milli>(clock::now() - t0).count();
    }
};

struct GpuTimer {
    GLuint q = 0;
    void init() { if (!q) glGenQueries(1, &q); }
    void begin() { init(); glBeginQuery(GL_TIME_ELAPSED, q); }
    double end_ms() {
        glEndQuery(GL_TIME_ELAPSED);
        GLuint64 ns = 0;
        glGetQueryObjectui64v(q, GL_QUERY_RESULT, &ns); // 简单起见：阻塞读取
        return ns / 1e6; // ns -> ms
    }
};

struct Section {
    CpuTimer cpu;
    GpuTimer gpu;
    double cpu_ms_accum = 0.0;
    double gpu_ms_accum = 0.0;
    int     samples = 0;
};

class Profiler {
public:
    void frameBegin() { ++frame_; }
    void frameEnd(int print_every = 120) {
        if (frame_ % print_every == 0) { print(); clear(); }
    }

    Section& sec(const std::string& name) { return sections_[name]; }

    // 跨函数用：begin/end
    void begin(const char* name) {
        Section& s = sec(name);
        s.cpu.begin();
        s.gpu.begin();
    }
    void end(const char* name) {
        Section& s = sec(name);
        s.cpu_ms_accum += s.cpu.end_ms();
        s.gpu_ms_accum += s.gpu.end_ms();
        s.samples++;
    }

    void print() {
        std::puts("=== Profiling (avg per section) ===");
        for (auto& kv : sections_) {
            const auto& k = kv.first;
            const auto& s = kv.second;
            const double c = s.samples ? (s.cpu_ms_accum / s.samples) : 0.0;
            const double g = s.samples ? (s.gpu_ms_accum / s.samples) : 0.0;
            std::printf("  %-22s CPU %6.3f ms | GPU %6.3f ms\n", k.c_str(), c, g);
        }
    }
    void clear() { for (auto& kv : sections_) kv.second = Section{}; }

private:
    std::unordered_map<std::string, Section> sections_;
    int frame_ = 0;
};

extern Profiler gProfiler;

// —— RAII scope（同一函数里包一段代码时更方便）——
struct CpuGpuScope {
    Section& s;
    CpuGpuScope(Section& sec) : s(sec) { s.cpu.begin(); s.gpu.begin(); }
    ~CpuGpuScope() { s.cpu_ms_accum += s.cpu.end_ms(); s.gpu_ms_accum += s.gpu.end_ms(); s.samples++; }
};

#define PROFILE_FRAME_BEGIN() gProfiler.frameBegin()
#define PROFILE_FRAME_END()   gProfiler.frameEnd()
#define PROFILE_BEGIN(name)   gProfiler.begin(name)
#define PROFILE_END(name)     gProfiler.end(name)
#define PROFILE_CPU_GPU(name) CpuGpuScope __cpugpu_scope_##__LINE__( gProfiler.sec(name) )
