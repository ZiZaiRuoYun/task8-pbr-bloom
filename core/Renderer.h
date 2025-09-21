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
    // �̶���λ���ܿ� 10/11/12 �� IBL��
    enum { TEX_ALBEDO = 20, TEX_NORMAL = 21, TEX_MRAO = 22, TEX_EMISSIVE = 23 };

    // ��������
    GLuint defaultWhiteSRGB_ = 0;
    GLuint defaultBlack_ = 0;
    GLuint defaultNormal_ = 0;
    GLuint defaultMRAO_ = 0;

    // ����ѡ��һ����С���ʽṹ����Ҳ�������Լ��� ECS/Component
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

    // ==== ECS ·�� ====
    void Begin(const glm::mat4& view, const glm::mat4& projection);
    void Submit(const glm::mat4& model);   // �ռ�һ��ʵ��� model ����
    void Submit(const glm::mat4& model, const MaterialPBR& m);
    void End();                            // һ������ instanced draw + ��Ļ�ռ����

    // ==== ���� ====
    // 0=Albedo, 1=Normal, 2=Position, 3=Spec������ view_gbuffer.frag ����չ��
    void SetDebugMode(int m) { debugMode_ = m; }

    // ==== �ƹ� UBO�������� lighting.frag / light_marker.* ���룩====
    Shader lightingProg_;
    GLuint lightsUBO_ = 0;
    static constexpr int MAX_LIGHTS = 256;

    struct PointLightCPU {
        glm::vec4 pos_radius;       // xyz + radius
        glm::vec4 color_intensity;  // rgb + intensity
    };
    std::vector<PointLightCPU> lights_;

    // ��¼���λ�ã��� Begin ��ͨ�� view ������õ���
    glm::vec3 camPosWS_{ 0.f };

    // ==== �������� ====
    float ambient_ = 0.10f;
    float radiusScale_ = 1.20f;
    float intensityScale_ = 1.30f;
    float specPowMin_ = 32.0f;
    float specPowMax_ = 256.0f;
    float exposure_ = 1.6f;
    int   useGamma_ = 0;    // Gamma(2.2)
    int   useACES_ = 1;    // ACES���� Gamma ��ѡһ��
    int   lightCountOverride_ = -1;   // -1 ��ȫ���������ô�ֵ����MAX_LIGHTS��

    // ��˸���
    std::vector<float>    baseIntensity_;
    std::vector<glm::vec4> colIHost_;
    float flickerAmp_ = 0.30f;
    float flickerSpeed_ = 3.0f;
    bool  flickerOn_ = true;

    // UBO Ƭ�γߴ磨pos �� col ���α�������
    GLsizeiptr uboSizePos_ = 0;
    GLsizeiptr uboSizeCol_ = 0;

    // �Ʊ�ǣ����ӻ���⣩
    GLuint lightMarkerVAO_ = 0, lightMarkerVBO_ = 0;
    Shader lightMarkerProg_;
    float  lightMarkerSize_ = 0.35f;

    // ��֡�������ں��� pass��
    glm::mat4 currentView_{ 1.f }, currentProj_{ 1.f }, currentViewInv_{ 1.f };

    // ��Ч
    bool  fogOn_ = false;
    float fogDensity_ = 0.02f;
    glm::vec3 fogColor_{ 0.02f, 0.02f, 0.05f };
    ~Renderer();

private:
    // Forward ·���õ�С shader��DrawScene ʹ�ã�
    Shader shader;

    // һ��ʾ��������֧��ʵ������VAO �ں�λ�õ����ԣ�
    Mesh cubeMesh;

    // ʵ������ VBO + CPU ��ʵ���б�
    unsigned int            instanceVBO = 0;
    std::vector<glm::mat4>  modelMatrices;

    // === �ӳ���Ⱦ ===
    GBuffer gbuf_;          // G-Buffer��λ�á����ߡ�AlbedoSpec��
    Shader  geomProg_;      // ���ν׶Σ�geometry.vert + geometry.frag��A�棺gAlbedoSpec.a=roughness��
    Shader  blitProg_;      // С���ߣ�blit.vert + view_gbuffer.frag��GBuffer ���ӻ���
    Shader  pbrProg_;       // ���ս׶Σ�blit.vert + pbr_lighting.frag��A�棩
    Shader  viewGBufferProg_; // ���ã��������ڱ�ʹ�ã�
    unsigned int screenVAO_ = 0; // �� gl_VertexID ����ȫ������ʱ��Ҫһ���� VAO

    Shader gbufProg_;

    int debugMode_ = -1;

    GLuint gBRDFLUT_ = 0;   // 2D LUT ����
    Shader brdfLUTProg_;    // brdf_lut ��ɫ��
    void GenerateBRDFLUT(int size); // ����

    // HDR offscreen
    GLuint hdrFBO_ = 0;
    GLuint hdrColor_ = 0;
    int hdrW_ = 0, hdrH_ = 0;

    void initHDR(int w, int h);
    void destroyHDR();
    void ensureHDRSize(int w, int h);

    // ������ջ��
    std::unique_ptr<PostProcessor> post_;

};
