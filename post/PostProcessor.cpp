#include "post/PostProcessor.h"
#include "post/effects/ToneMapping.h"
#include "profiling/Profiler.h"

PostProcessor::PostProcessor(int w, int h) : w_(w), h_(h) {
	tonemap_ = std::make_unique<ToneMapEffect>();
}

void PostProcessor::resize(int w, int h) {
    w_ = w; h_ = h;
    if (bloom_) bloom_->resize(w_, h_);
}


void PostProcessor::run(GLuint hdrTex, GLuint gMaskTex, int screenW, int screenH, GLuint fsVAO)
{
    bool   useBloom = false;
    GLuint bloomTex = 0;

    if (bloomEnable && bloom_) {
        // 尺寸变化时顺手同步一下 bloom 的内部 RT
        if (w_ != screenW || h_ != screenH) { w_ = screenW; h_ = screenH; bloom_->resize(w_, h_); }

        // 在 HDR（tone map 之前）做亮部抽取与模糊
        bloomTex = bloom_->process(hdrTex, screenW, screenH, fsVAO);
        useBloom = (bloomTex != 0);
    }

    // 传入曝光 / 模式 / gamma
    tonemap_->setParams(tmExposure, tmMode, tmGamma);
    {
        PROFILE_CPU_GPU("Tonemap");
        tonemap_->process(hdrTex, gMaskTex, screenW, screenH, fsVAO, useBloom, bloomTex, bloomStrength);
    }
}


PostProcessor::~PostProcessor() = default;

void PostProcessor::setBloom(bool enable,
    float threshold,
    float knee,
    int   iters,
    float strength,
    int   downscale)
{
    // 开关 & Tonemap端使用的强度（在 ToneMapEffect::process 里作为 uBloomStrength）
    bloomEnable = enable;
    bloomStrength = strength;

    // 关掉就不需要后续初始化
    if (!enable) return;

    // 合理范围的参数钳制
    downscale = std::max(1, downscale);
    iters = std::max(1, iters);
    knee = glm::clamp(knee, 0.0f, 1.0f);
    threshold = std::max(0.0f, threshold);

    // 初始化或更新 BloomEffect
    if (!bloom_) {
        bloom_ = std::make_unique<BloomEffect>();
        bloom_->init(std::max(1, w_), std::max(1, h_), downscale);
    }
    else {
        // 如果降采样系数变化，更新并重新分配
        if (bloom_->downscale != downscale) {
            bloom_->downscale = downscale;
            bloom_->resize(w_, h_);
        }
    }

    // 写入运行参数（BloomEffect::process 会读取）
    bloom_->threshold = threshold;
    bloom_->knee = knee;
    bloom_->iterations = iters;
    bloom_->strength = strength;
}
