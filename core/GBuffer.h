#pragma once
#include <glad/glad.h>

// 说明：为兼容旧管线，保留 gPosition / gNormal / gAlbedoSpec；
// 同时新增 PBR 所需的独立附件：gAlbedo / gNormalRough / gMetalAO / gEmissive。
class GBuffer {
public:
    // 初始化 / 尺寸变更
    bool init(int width, int height);
    void resize(int width, int height);

    // 绑定：几何阶段写入 / 光照阶段读取（采样）
    void bindForGeometryPass(); 
    void bindForReading();

    // 旧附件（兼容 Blinn–Phong）
    GLuint getPosition() const { return gPosition_; }
    GLuint getNormal()   const { return gNormal_; }
    GLuint getAlbedoSpec() const { return gAlbedoSpec_; }

    // 帧缓冲/FBO 与实用函数
    GLuint fbo() const { return fbo_; }
    void blitDepthToDefault(int dstW, int dstH);

    // NEW: PBR 附件
    // Albedo: 基础色（独立于 legacy gAlbedoSpec）
    GLuint getAlbedo()        const { return gAlbedo_; }
    // NormalRough: xyz = 法线（线性存储，或 [-1,1] 映射由 shader 负责），a = roughness
    GLuint getNormalRough()   const { return gNormalRough_; }
    // MetalAO: r = metallic, g = ao
    GLuint getMetalAO()       const { return gMetalAO_; }
    // Emissive: 发光项（可选使用）
    GLuint getEmissive()      const { return gEmissive_; }

private:
    void destroy();
    void allocTextures(int w, int h);   // 内部分配/重新分配所有附件

    // FBO / 深度
    GLuint fbo_ = 0, rboDepth_ = 0;

    // 旧附件
    GLuint gPosition_ = 0; // RGBA16F
    GLuint gNormal_ = 0; // RGBA16F
    GLuint gAlbedoSpec_ = 0; // RGBA8（a 通道曾放 spec）

    // NEW: PBR 附件
    GLuint gAlbedo_ = 0; // SRGB8_ALPHA8 或 RGBA8（写入走线性→输出阶段统一做 gamma）
    GLuint gNormalRough_ = 0; // RGBA16F（xyz=normal, a=roughness）
    GLuint gMetalAO_ = 0; // RG8 / RG16F（r=metallic, g=ao）
    GLuint gEmissive_ = 0; // RGB16F（可选）

    int w_ = 0, h_ = 0;
};

