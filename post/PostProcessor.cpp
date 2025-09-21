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
        // �ߴ�仯ʱ˳��ͬ��һ�� bloom ���ڲ� RT
        if (w_ != screenW || h_ != screenH) { w_ = screenW; h_ = screenH; bloom_->resize(w_, h_); }

        // �� HDR��tone map ֮ǰ����������ȡ��ģ��
        bloomTex = bloom_->process(hdrTex, screenW, screenH, fsVAO);
        useBloom = (bloomTex != 0);
    }

    // �����ع� / ģʽ / gamma
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
    // ���� & Tonemap��ʹ�õ�ǿ�ȣ��� ToneMapEffect::process ����Ϊ uBloomStrength��
    bloomEnable = enable;
    bloomStrength = strength;

    // �ص��Ͳ���Ҫ������ʼ��
    if (!enable) return;

    // ����Χ�Ĳ���ǯ��
    downscale = std::max(1, downscale);
    iters = std::max(1, iters);
    knee = glm::clamp(knee, 0.0f, 1.0f);
    threshold = std::max(0.0f, threshold);

    // ��ʼ������� BloomEffect
    if (!bloom_) {
        bloom_ = std::make_unique<BloomEffect>();
        bloom_->init(std::max(1, w_), std::max(1, h_), downscale);
    }
    else {
        // ���������ϵ���仯�����²����·���
        if (bloom_->downscale != downscale) {
            bloom_->downscale = downscale;
            bloom_->resize(w_, h_);
        }
    }

    // д�����в�����BloomEffect::process ���ȡ��
    bloom_->threshold = threshold;
    bloom_->knee = knee;
    bloom_->iterations = iters;
    bloom_->strength = strength;
}
