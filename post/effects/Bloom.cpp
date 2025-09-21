#include "post/effects/Bloom.h"
#include "Shader.h"
#include "profiling/Profiler.h"

bool BloomEffect::init(int w, int h, int d) {
    downscale = d;
    extract_ = std::make_unique<Shader>();
    blur_ = std::make_unique<Shader>();
    extract_->Load("shaders/blit.vert", "shaders/pp/bloom_extract.frag");
    blur_->Load("shaders/blit.vert", "shaders/pp/gauss_blur.frag");
    alloc(w, h);
    return true;
}
void BloomEffect::alloc(int w, int h) {
    w_ = std::max(1, w / downscale);
    h_ = std::max(1, h / downscale);

    glGenFramebuffers(2, fbo_);
    glGenTextures(2, tex_);
    for (int i = 0; i < 2; ++i) {
        glBindTexture(GL_TEXTURE_2D, tex_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w_, h_, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void BloomEffect::resize(int w, int h) {
    // 简化：直接销毁重建（生产可做更优重用）
    if (fbo_[0]) glDeleteFramebuffers(2, fbo_), glDeleteTextures(2, tex_);
    alloc(w, h);
}
GLuint BloomEffect::process(GLuint hdrTex, int screenW, int screenH, GLuint fsVAO)
{
    glDisable(GL_DEPTH_TEST);

    // --- Bright pass ---
    {
        PROFILE_CPU_GPU("Bloom-Bright");
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[0]);         // tex_[0] = bright
        glViewport(0, 0, w_, h_);
        extract_->Use();
        extract_->SetInt("uHDR", 0);
        extract_->SetFloat("uThreshold", threshold);
        extract_->SetFloat("uKnee", knee);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, hdrTex);

        glBindVertexArray(fsVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
    }

    // --- Blur (H/V ping-pong) ---
    {
        PROFILE_CPU_GPU("Bloom-Blur");
        bool horizontal = true;
        for (int i = 0; i < iterations * 2; ++i) {
            int dst = horizontal ? 1 : 0;
            int src = horizontal ? 0 : 1;

            glBindFramebuffer(GL_FRAMEBUFFER, fbo_[dst]);
            glViewport(0, 0, w_, h_);
            blur_->Use();
            blur_->SetInt("uImage", 0);
            blur_->SetInt("uHorizontal", horizontal ? 1 : 0);
            blur_->SetVec2("uTexelSize", glm::vec2(1.0f / w_, 1.0f / h_));
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex_[src]);

            glDrawArrays(GL_TRIANGLES, 0, 3);
            horizontal = !horizontal;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(0);

        // 返回最后一次写入的贴图
        return (iterations % 1 == 0) ? tex_[0] : tex_[1]; // N=iterations*2 为偶数，等价于返回 tex_[0]
    }
}

