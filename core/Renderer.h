#pragma once
#include <vector>
#include <glm/glm.hpp>
#include "SceneNode.h"
#include "Shader.h"
#include "Mesh.h"
#include "GBuffer.h"
#include "post/PostProcessor.h" 

struct MaterialPBR;

class PostProcessor;

class Renderer {
    // 固定槽位（避开 10/11/12 的 IBL）
    enum { TEX_ALBEDO = 20, TEX_NORMAL = 21, TEX_MRAO = 22, TEX_EMISSIVE = 23 };

    // 兜底纹理
    GLuint defaultWhiteSRGB_ = 0;
    GLuint defaultBlack_ = 0;
    GLuint defaultNormal_ = 0;
    GLuint defaultMRAO_ = 0;

    // （可选）一个最小材质结构，你也可以用自己的 ECS/Component
    struct PBRMaterial {
        glm::vec3 albedo = { 1.0f, 0.7f, 0.3f };
        float     roughness = 0.5f;
        float     metallic = 0.0f;
        float     ao = 1.0f;
        glm::vec3 emissive = { 0.0f, 0.0f, 0.0f };

        GLuint    albedoTex = 0;
        GLuint    normalTex = 0;
        GLuint    mraoTex = 0;  // RGB: M,R,AO
        GLuint    emissiveTex = 0;
    };

    void createDefaultTextures();                       
    void bindMaterialForGBuffer(const PBRMaterial& m);


public:
    void Init();
    void DrawScene(SceneNode* root);

    // ==== ECS 路径 ====
    void Begin(const glm::mat4& view, const glm::mat4& projection);
    void Submit(const glm::mat4& model);   // 收集一个实体的 model 矩阵
    void Submit(const glm::mat4& model, const MaterialPBR& m);
    void End();                            // 一次性做 instanced draw + 屏幕空间光照

    // ==== 调试 ====
    // 0=Albedo, 1=Normal, 2=Position, 3=Spec（可在 view_gbuffer.frag 中扩展）
    void SetDebugMode(int m) { debugMode_ = m; }

    // ==== 灯光 UBO（与现有 lighting.frag / light_marker.* 对齐）====
    Shader lightingProg_;
    GLuint lightsUBO_ = 0;
    static constexpr int MAX_LIGHTS = 256;

    struct PointLightCPU {
        glm::vec4 pos_radius;       // xyz + radius
        glm::vec4 color_intensity;  // rgb + intensity
    };
    std::vector<PointLightCPU> lights_;

    // 记录相机位置（由 Begin 里通过 view 矩阵逆得到）
    glm::vec3 camPosWS_{ 0.f };

    // ==== 参数调节 ====
    float ambient_ = 0.10f;
    float radiusScale_ = 1.20f;
    float intensityScale_ = 1.30f;
    float specPowMin_ = 32.0f;
    float specPowMax_ = 256.0f;
    float exposure_ = 1.6f;
    int   useGamma_ = 0;    // Gamma(2.2)
    int   useACES_ = 1;    // ACES（与 Gamma 二选一）
    int   lightCountOverride_ = -1;   // -1 用全部，否则用此值（≤MAX_LIGHTS）

    // 闪烁相关
    std::vector<float>    baseIntensity_;
    std::vector<glm::vec4> colIHost_;
    float flickerAmp_ = 0.30f;
    float flickerSpeed_ = 3.0f;
    bool  flickerOn_ = true;

    // UBO 片段尺寸（pos 与 col 两段背靠背）
    GLsizeiptr uboSizePos_ = 0;
    GLsizeiptr uboSizeCol_ = 0;

    // 灯标记（可视化点光）
    GLuint lightMarkerVAO_ = 0, lightMarkerVBO_ = 0;
    Shader lightMarkerProg_;
    float  lightMarkerSize_ = 0.35f;

    // 本帧矩阵（用于后续 pass）
    glm::mat4 currentView_{ 1.f }, currentProj_{ 1.f }, currentViewInv_{ 1.f };

    // 雾效
    bool  fogOn_ = false;
    float fogDensity_ = 0.02f;
    glm::vec3 fogColor_{ 0.02f, 0.02f, 0.05f };
    ~Renderer();

private:
    // Forward 路径用的小 shader（DrawScene 使用）
    Shader shader;

    // 一个示例网格（已支持实例化，VAO 内含位置等属性）
    Mesh cubeMesh;

    // 实例化用 VBO + CPU 侧实例列表
    unsigned int            instanceVBO = 0;
    std::vector<glm::mat4>  modelMatrices;

    // === 延迟渲染 ===
    GBuffer gbuf_;          // G-Buffer（位置、法线、AlbedoSpec）
    Shader  geomProg_;      // 几何阶段：geometry.vert + geometry.frag（A版：gAlbedoSpec.a=roughness）
    Shader  blitProg_;      // 小工具：blit.vert + view_gbuffer.frag（GBuffer 可视化）
    Shader  pbrProg_;       // 光照阶段：blit.vert + pbr_lighting.frag（A版）
    Shader  viewGBufferProg_; // 备用（若你已在别处使用）
    unsigned int screenVAO_ = 0; // 用 gl_VertexID 生成全屏三角时需要一个空 VAO

    Shader gbufProg_;

    int debugMode_ = -1;

    GLuint gBRDFLUT_ = 0;   // 2D LUT 纹理
    Shader brdfLUTProg_;    // brdf_lut 着色器
    void GenerateBRDFLUT(int size); // 声明

    // HDR offscreen
    GLuint hdrFBO_ = 0;
    GLuint hdrColor_ = 0;
    int hdrW_ = 0, hdrH_ = 0;

    void initHDR(int w, int h);
    void destroyHDR();
    void ensureHDRSize(int w, int h);

    // （后期栈）
    std::unique_ptr<PostProcessor> post_;

};
