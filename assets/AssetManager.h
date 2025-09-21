#pragma once
#include <unordered_map>
#include <memory>
#include <string>
#include <mutex>
#include <queue>
#include <functional>

#include "Handle.h"
#include "AsyncLoader.h"
#include "Model.h"

namespace assets {

    class AssetManager {
    public:
        static AssetManager& Instance();

        template<typename T>
        Handle<T> LoadAsync(const std::string& path);

        template<typename T>
        std::shared_ptr<T> Get(const std::string& path);

        void TickMainThread();

    private:
        AssetManager() = default;

        struct Entry { std::shared_ptr<Resource> res; };

        std::unordered_map<std::string, Entry> cache_;
        std::mutex cache_mtx_;
        AsyncLoader loader_;

        // 主线程任务队列
        std::mutex main_mtx_;
        std::queue<std::function<void()>> main_jobs_;
        void PostToMain(std::function<void()> job) {
            std::lock_guard<std::mutex> lk(main_mtx_);
            main_jobs_.push(std::move(job));
        }

        // === NEW: 后台 Assimp 解析（只做 CPU 数据）===
        static bool LoadModelCPU_Assimp(const std::string& path,
            const std::shared_ptr<Model>& outModel,
            std::string& errMsg);
    };

    // ===== 模板实现=====
    inline AssetManager& AssetManager::Instance() { static AssetManager inst; return inst; }

    template<typename T>
    inline std::shared_ptr<T> AssetManager::Get(const std::string& path) {
        std::lock_guard<std::mutex> lk(cache_mtx_);
        auto it = cache_.find(path);
        if (it == cache_.end()) return nullptr;
        return std::static_pointer_cast<T>(it->second.res);
    }

    template<typename T>
    inline Handle<T> AssetManager::LoadAsync(const std::string& path) {
        {   // 命中缓存
            std::lock_guard<std::mutex> lk(cache_mtx_);
            auto it = cache_.find(path);
            if (it != cache_.end())
                return Handle<T>(std::static_pointer_cast<T>(it->second.res));
        }

        auto res = std::make_shared<T>();
        {
            std::lock_guard<std::mutex> lk(cache_mtx_);
            cache_[path] = Entry{ res };
        }

        // === 后台任务 ===
        loader_.Enqueue([this, res, path] {
            try {
                std::string err;
                bool ok = true;

                if constexpr (std::is_same_v<T, Model>) {
                    // 1) 后台用 Assimp 解析，仅填 CPU 数据
                    ok = LoadModelCPU_Assimp(path, res, err);

                    if (!ok) {
                        res->state.store(ResourceState::Failed);
                        std::cerr << "[Asset] Assimp load failed: " << err << "\n";
                        return;
                    }

                    // 2) 把 GL 上传放到主线程
                    this->PostToMain([res] {
                        // 为每个 submesh 创建 GPU 资源
                        res->meshesGPU.resize(res->meshesCPU.size());
                        for (std::size_t i = 0; i < res->meshesCPU.size(); ++i) {
                            const auto& cpu = res->meshesCPU[i];
                            auto& gpu = res->meshesGPU[i];

                            glGenVertexArrays(1, &gpu.vao);
                            glGenBuffers(1, &gpu.vbo);
                            glGenBuffers(1, &gpu.ebo);

                            glBindVertexArray(gpu.vao);

                            glBindBuffer(GL_ARRAY_BUFFER, gpu.vbo);
                            glBufferData(GL_ARRAY_BUFFER,
                                cpu.vertices.size() * sizeof(Vertex),
                                cpu.vertices.data(),
                                GL_STATIC_DRAW);

                            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gpu.ebo);
                            glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                                cpu.indices.size() * sizeof(std::uint32_t),
                                cpu.indices.data(),
                                GL_STATIC_DRAW);

                            // layout:
                            // 0: position (vec3), 1: normal (vec3), 2: uv (vec2)
                            GLsizei stride = sizeof(Vertex);
                            // pos
                            glEnableVertexAttribArray(0);
                            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, pos));
                            // normal
                            glEnableVertexAttribArray(1);
                            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, normal));
                            // uv
                            glEnableVertexAttribArray(2);
                            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(Vertex, uv));

                            glBindVertexArray(0);

                            gpu.indexCount = cpu.indices.size();
                        }

                        res->gpu_uploaded.store(true);
                        res->state.store(ResourceState::Ready);
                        });
                }
                else {
                    // 其它类型的资源……（保留以后扩展）
                }
            }
            catch (...) {
                res->state.store(ResourceState::Failed);
            }
            });

        return Handle<T>(res);
    }

} // namespace assets
