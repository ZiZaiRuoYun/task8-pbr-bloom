// GBuffer.cpp
#include "GBuffer.h"
#include <cassert>

static void setupTex2D(GLuint tex, GLint internalFmt,
    GLsizei w, GLsizei h,
    GLenum fmt, GLenum type,
    GLenum filter = GL_NEAREST)
{
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, internalFmt, w, h, 0, fmt, type, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

bool GBuffer::init(int width, int height) {
    destroy();

    w_ = width;
    h_ = height;

    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // --- 生成所有颜色附件（包含 legacy 与 PBR 新附件） ---
    glGenTextures(1, &gPosition_);
    glGenTextures(1, &gNormal_);
    glGenTextures(1, &gAlbedoSpec_);

    // NEW:
    glGenTextures(1, &gAlbedo_);
    glGenTextures(1, &gNormalRough_);
    glGenTextures(1, &gMetalAO_);
    glGenTextures(1, &gEmissive_);

    // 一次性分配所有纹理存储
    allocTextures(w_, h_);

    // --- 绑定到 MRT 槽位 ---
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec_, 0);

    // NEW: PBR 附件
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT3, GL_TEXTURE_2D, gAlbedo_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT4, GL_TEXTURE_2D, gNormalRough_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT5, GL_TEXTURE_2D, gMetalAO_, 0);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT6, GL_TEXTURE_2D, gEmissive_, 0);

    // 深度 Renderbuffer
    glGenRenderbuffers(1, &rboDepth_);
    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w_, h_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rboDepth_);

    // 默认启用全部颜色附件输出（几何阶段会调用 glDrawBuffers）
    static const GLenum bufs[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6
    };
    glDrawBuffers((GLsizei)(sizeof(bufs) / sizeof(bufs[0])), bufs);

    bool ok = (glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

// 统一重新分配/初次分配所有附件纹理的存储
void GBuffer::allocTextures(int w, int h) {
    // Legacy
    setupTex2D(gPosition_, GL_RGBA16F, w, h, GL_RGBA, GL_FLOAT);         // 位置（世界/视空间）
    setupTex2D(gNormal_, GL_RGBA16F, w, h, GL_RGBA, GL_FLOAT);         // 法线（legacy）
    setupTex2D(gAlbedoSpec_, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE); // 旧的 Albedo+Spec（a 通道曾存 spec）

    // NEW: PBR 附件
    // 1) Albedo：若上下文支持 sRGB framebuffer，可用 GL_SRGB8_ALPHA8；否则用 RGBA8，并在 shader 里统一走线性空间
//#ifdef GL_SRGB8_ALPHA8
//    setupTex2D(gAlbedo_, GL_SRGB8_ALPHA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
//#else
//    setupTex2D(gAlbedo_, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE);
//#endif

    setupTex2D(gAlbedo_, GL_RGBA8, w, h, GL_RGBA, GL_UNSIGNED_BYTE);

    // 2) NormalRough：高精度存储法线与粗糙度
    setupTex2D(gNormalRough_, GL_RGBA16F, w, h, GL_RGBA, GL_FLOAT);

    // 3) MetalAO：节省带宽，优先用 8-bit；若需要更高精度可切换 GL_RG16F
    setupTex2D(gMetalAO_, GL_RG8, w, h, GL_RG, GL_UNSIGNED_BYTE);

    // 4) Emissive：线性高动态
    setupTex2D(gEmissive_, GL_RGB16F, w, h, GL_RGB, GL_FLOAT);
}

void GBuffer::resize(int width, int height) {
    if (width == w_ && height == h_) return;
    w_ = width; h_ = height;

    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    allocTextures(w_, h_);  // 重新分配所有颜色附件

    glBindRenderbuffer(GL_RENDERBUFFER, rboDepth_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w_, h_);

    // 维持相同的 MRT 列表
    static const GLenum bufs[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6
    };
    glDrawBuffers((GLsizei)(sizeof(bufs) / sizeof(bufs[0])), bufs);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void GBuffer::bindForGeometryPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, w_, h_);

    // 写入所有颜色附件（包含 legacy）
    static const GLenum bufs[] = {
        GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2,
        GL_COLOR_ATTACHMENT3, GL_COLOR_ATTACHMENT4, GL_COLOR_ATTACHMENT5,
        GL_COLOR_ATTACHMENT6
    };
    glDrawBuffers((GLsizei)(sizeof(bufs) / sizeof(bufs[0])), bufs);

    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void GBuffer::bindForReading() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
}

void GBuffer::destroy() {
    if (gPosition_)    glDeleteTextures(1, &gPosition_);
    if (gNormal_)      glDeleteTextures(1, &gNormal_);
    if (gAlbedoSpec_)  glDeleteTextures(1, &gAlbedoSpec_);

    // NEW: PBR 附件清理
    if (gAlbedo_)      glDeleteTextures(1, &gAlbedo_);
    if (gNormalRough_) glDeleteTextures(1, &gNormalRough_);
    if (gMetalAO_)     glDeleteTextures(1, &gMetalAO_);
    if (gEmissive_)    glDeleteTextures(1, &gEmissive_);

    if (rboDepth_)     glDeleteRenderbuffers(1, &rboDepth_);
    if (fbo_)          glDeleteFramebuffers(1, &fbo_);

    fbo_ = rboDepth_ = 0;
    gPosition_ = gNormal_ = gAlbedoSpec_ = 0;
    gAlbedo_ = gNormalRough_ = gMetalAO_ = gEmissive_ = 0;
}

void GBuffer::blitDepthToDefault(int dstW, int dstH) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo_);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, w_, h_, 0, 0, dstW, dstH, GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}