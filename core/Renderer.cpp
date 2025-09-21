#include "Renderer.h"
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <GLFW/glfw3.h>
#include <cassert>
#include <stack>
#include <filesystem>
#include <iostream>
#include <algorithm>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include "../components/MaterialPBR.h"
#include "post/PostProcessor.h" 
#include "profiling/Profiler.h"

//extern Shader gbufProg_;

Renderer::~Renderer() = default;

static GLuint gEnvCubemap = 0;
static GLuint gIBLFBO = 0, gIBLRBO = 0;
static Shader equirect2cubeProg;
static Shader skyboxProg;

static const int ENV_CUBE_SIZE = 512; // �� 512/1024
static const char* kHDRPath = "assets/hdr/simple_sky.hdr";
static const char* kEq2CubeVS = "shaders/equirect2cube.vert";
static const char* kEq2CubeFS = "shaders/equirect2cube.frag";
static const char* kSkyboxVS = "shaders/skybox.vert";
static const char* kSkyboxFS = "shaders/skybox.frag";
static Shader debugFaceProg;
static GLuint gIrradianceMap = 0;
static Shader irradianceProg;

bool gShowIrradiance = false;
int  gSkyboxPreviewMode = 0; // 0=off, 1=env, 2=irr
bool gDebugShowAlbedo = false; // �������Ƿ�ֱ�� GBuffer Albedo

static GLuint gPrefilterMap = 0;
static Shader prefilterProg;
// �� PBR shader ֪���ж��� mip
static int gPrefilterMaxMip = 0;

namespace {
    // HDR ��ȡ
    static unsigned int LoadHDRTexture(const std::string& path) {
        stbi_set_flip_vertically_on_load(true); // ������µߵ����е� false ����
        int w = 0, h = 0, nc = 0;
        float* data = stbi_loadf(path.c_str(), &w, &h, &nc, 0);
        if (!data) {
            std::cerr << "[IBL] Failed to load HDR: " << path << "\n";
            return 0;
        }
        GLuint tex = 0;
        glGenTextures(1, &tex);
        glBindTexture(GL_TEXTURE_2D, tex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, nc == 3 ? GL_RGB : GL_RGBA, GL_FLOAT, data);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        stbi_image_free(data);
        return tex;
    }

    // ��ʱ�����壨����պ�/���߲����ã�
    static GLuint gCubeVAO = 0, gCubeVBO = 0, gCubeEBO = 0;
    static void RenderUnitCube() {
        if (!gCubeVAO) {
            float v[] = {
                -1,-1,-1,  -1,-1, 1,  -1, 1, 1,  -1, 1,-1,
                 1,-1,-1,   1,-1, 1,   1, 1, 1,   1, 1,-1,
                -1,-1,-1,  -1,-1, 1,   1,-1, 1,   1,-1,-1,
                -1, 1,-1,  -1, 1, 1,   1, 1, 1,   1, 1,-1,
                -1,-1,-1,  -1, 1,-1,   1, 1,-1,   1,-1,-1,
                -1,-1, 1,  -1, 1, 1,   1, 1, 1,   1,-1, 1,
            };
            unsigned int idx[] = {
                0,1,2, 0,2,3,  4,5,6, 4,6,7,
                8,9,10, 8,10,11, 12,13,14, 12,14,15,
                16,17,18, 16,18,19, 20,21,22, 20,22,23
            };
            glGenVertexArrays(1, &gCubeVAO);
            glGenBuffers(1, &gCubeVBO);
            glGenBuffers(1, &gCubeEBO);
            glBindVertexArray(gCubeVAO);
            glBindBuffer(GL_ARRAY_BUFFER, gCubeVBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(v), v, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gCubeEBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(idx), idx, GL_STATIC_DRAW);
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
            glBindVertexArray(0);
        }
        glBindVertexArray(gCubeVAO);
        glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    // equirect �������
    static inline glm::vec2 SampleSphericalMap(const glm::vec3& v) {
        static const glm::vec2 INV_ATAN(0.1591f, 0.3183f); // 1/2��, 1/��
        glm::vec2 uv(std::atan2(v.z, v.x), std::asin(v.y));
        uv *= INV_ATAN; uv += 0.5f;
        return uv;
    }
} // namespace

void Renderer::Init() {
    //debugFaceProg.Load("shaders/cubeface_fullscreen.vert", "shaders/cubeface_fullscreen.frag");
    // === ������Դ ===
    cubeMesh.InitBuffers();
    glGenBuffers(1, &instanceVBO);
    glEnable(GL_DEPTH_TEST);

    // ֡����ߴ粢��ʼ�� GBuffer
    int fbw = 0, fbh = 0;
    GLFWwindow* cur = glfwGetCurrentContext();
    glfwGetFramebufferSize(cur, &fbw, &fbh);
    if (!gbuf_.init(fbw, fbh)) {
        assert(false && "GBuffer init failed");
    }

    // === ������ɫ������ ===
    geomProg_.Load("shaders/geometry.vert", "shaders/geometry.frag");
    blitProg_.Load("shaders/blit.vert", "shaders/view_gbuffer.frag");

    pbrProg_.Load("shaders/blit.vert", "shaders/pbr_lighting.frag");
    pbrProg_.Use();
    pbrProg_.SetInt("uGPosition", 0);
    pbrProg_.SetInt("uGNormalRough", 1);
    pbrProg_.SetInt("uGAlbedo", 2);
    pbrProg_.SetInt("uGMetalAO", 3);
    pbrProg_.SetInt("uGEmissive", 4);
    pbrProg_.SetInt("uIrradianceMap", 10);   // ����������Ԫ 10 �� irradiance
    pbrProg_.SetInt("uPrefilterMap", 11);
    pbrProg_.SetFloat("uPrefilterMaxMip", (float)gPrefilterMaxMip);

    glActiveTexture(GL_TEXTURE10);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gIrradianceMap);
    GLuint blk = glGetUniformBlockIndex(pbrProg_.Get(), "Lights");
    if (blk != GL_INVALID_INDEX) glUniformBlockBinding(pbrProg_.Get(), blk, 0);


    // blit �۲� GBuffer
    blitProg_.Load("shaders/blit.vert", "shaders/view_gbuffer.frag");
    blitProg_.Use();
    blitProg_.SetInt("uGPosition", 0);
    blitProg_.SetInt("uGNormalRough", 1);
    blitProg_.SetInt("uGAlbedo", 2);
    blitProg_.SetInt("uGMetalAO", 3);
    blitProg_.SetInt("uGEmissive", 4);
    blitProg_.SetInt("uGNormalLegacy", 5);
    blitProg_.SetInt("uGAlbedoSpec", 6);

    glDisable(GL_FRAMEBUFFER_SRGB);

    // ��Ļ������ VAO�����У�
    glGenVertexArrays(1, &screenVAO_);

    // ���� lighting��UBO �󶨺� 0��
    lightingProg_.Load("shaders/blit.vert", "shaders/lighting.frag");
    {
        GLuint blockIndex = glGetUniformBlockIndex(lightingProg_.Get(), "Lights");
        if (blockIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(lightingProg_.Get(), blockIndex, 0);
        }
    }

    //// light marker��С�����ӻ���
    //lightMarkerProg_.Load("shaders/light_marker.vert", "shaders/light_marker.frag");
    //{
    //    GLuint blk = glGetUniformBlockIndex(lightMarkerProg_.Get(), "Lights");
    //    if (blk != GL_INVALID_INDEX) glUniformBlockBinding(lightMarkerProg_.Get(), blk, 0);
    //}
    //{
    //    float quad[] = { -1.f,-1.f,  1.f,-1.f,  1.f, 1.f,
    //                     -1.f,-1.f,  1.f, 1.f, -1.f, 1.f };
    //    glGenVertexArrays(1, &lightMarkerVAO_);
    //    glGenBuffers(1, &lightMarkerVBO_);
    //    glBindVertexArray(lightMarkerVAO_);
    //    glBindBuffer(GL_ARRAY_BUFFER, lightMarkerVBO_);
    //    glBufferData(GL_ARRAY_BUFFER, sizeof(quad), quad, GL_STATIC_DRAW);
    //    glEnableVertexAttribArray(0);
    //    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    //    glBindVertexArray(0);
    //}

    gbufProg_.Load("shaders/gbuffer_pbr.vert", "shaders/gbuffer_pbr.frag");
    // �趨�̶� uniform binding
    gbufProg_.Use();
    gbufProg_.SetInt("uGPosition", 0);
    gbufProg_.SetInt("uGNormalRough", 1);
    gbufProg_.SetInt("uGAlbedo", 2);
    gbufProg_.SetInt("uGMetalAO", 3);
    gbufProg_.SetInt("uGEmissive", 4);

    gbufProg_.SetInt("uAlbedoMap", TEX_ALBEDO);
    gbufProg_.SetInt("uNormalMap", TEX_NORMAL);
    gbufProg_.SetInt("uMetalRoughAOMap", TEX_MRAO);
    gbufProg_.SetInt("uEmissiveMap", TEX_EMISSIVE);

    createDefaultTextures();

    // === IBL Step1: HDR �� Cubemap �������޳�/���״̬�޸� + FBO ��顱��===
    // 1) ����ת��/��պ���ɫ��
    equirect2cubeProg.Load(kEq2CubeVS, kEq2CubeFS);
    skyboxProg.Load(kSkyboxVS, kSkyboxFS);

    // 2) ��ȡ HDR
    GLuint hdrTex = LoadHDRTexture(kHDRPath);
    assert(hdrTex && "HDR load failed");

    // 3) Ŀ�껷����������ͼ
    glGenTextures(1, &gEnvCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            ENV_CUBE_SIZE, ENV_CUBE_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 4) ���� FBO/RBO
    glGenFramebuffers(1, &gIBLFBO);
    glGenRenderbuffers(1, &gIBLRBO);
    glBindFramebuffer(GL_FRAMEBUFFER, gIBLFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gIBLRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, ENV_CUBE_SIZE, ENV_CUBE_SIZE);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, gIBLRBO);

    // 5) ��ͼ/ͶӰ����6 ������
    glm::mat4 proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 views[6] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)), // +X
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)), // -X
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)), // +Y
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)), // -Y
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)), // +Z
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0))  // -Z
    };

    // 6) ��Ⱦ 6 ���棨�޸����ر�/�����޳� + ���״̬����� FBO ��飩
    equirect2cubeProg.Use();
    equirect2cubeProg.SetInt("uEquirect", 0);
    equirect2cubeProg.SetMat4("uProjection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, hdrTex);

    glViewport(0, 0, ENV_CUBE_SIZE, ENV_CUBE_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, gIBLFBO);

    // ������ �����״̬
    GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
    GLint     prevCullMode = GL_BACK;
    if (prevCull) glGetIntegerv(GL_CULL_FACE_MODE, &prevCullMode);
    GLboolean prevDepth = glIsEnabled(GL_DEPTH_TEST);
    GLint     prevDepthFunc = GL_LESS;
    glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFunc);

    // ������ �ؼ����������塰�ڲ�������Ĭ���޳������ѿɼ����޵� �� ��
    glDisable(GL_CULL_FACE);              // ��glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    // FBO �����Լ�飨������ֱ����־��
    GLenum st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[IBL] capture FBO incomplete: 0x" << std::hex << st << std::dec << "\n";
    }

    for (int i = 0; i < 6; ++i) {
        equirect2cubeProg.SetMat4("uView", views[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, gEnvCubemap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUnitCube(); // ��Ⱦ�������ڱ�
    }

    // ������ �ָ���״̬
    if (prevCull) { glEnable(GL_CULL_FACE); glCullFace(prevCullMode); }
    else { glDisable(GL_CULL_FACE); }
    if (prevDepth) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(prevDepthFunc);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 7) ���� mipmap������ prefilter Ҫ�ã�
    glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // ---- ���ԣ��� +X ��ҵ� capture FBO���� 1��1 ���أ�ȷ�ϲ���ȫ 0 ----
    glBindFramebuffer(GL_FRAMEBUFFER, gIBLFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_CUBE_MAP_POSITIVE_X, gEnvCubemap, 0);

    st = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (st != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "[CHK] capture FBO incomplete: 0x" << std::hex << st << std::dec << "\n";
    }

    glViewport(0, 0, 1, 1);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    float pix[3] = { 0,0,0 };
    glReadPixels(0, 0, 1, 1, GL_RGB, GL_FLOAT, pix);
    std::cout << "[CHK] cubemap +X (0,0) = " << pix[0] << "," << pix[1] << "," << pix[2] << "\n";

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    std::cout << "[IBL] Environment cubemap generated (HDR->Cubemap done)\n";

    const int IRR_SIZE = 32; // �ͷֱ����㹻��32 �� 64

    // 1) ���ؾ����ɫ��
    irradianceProg.Load("shaders/irradiance_convolve.vert",
        "shaders/irradiance_convolve.frag");

    // 2) Ŀ�� irradiance cubemap���� mip��
    glGenTextures(1, &gIrradianceMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gIrradianceMap);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            IRR_SIZE, IRR_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // 3) ���� gIBLFBO/gIBLRBO ���в��񣬸ĳ� 32��32
    glBindFramebuffer(GL_FRAMEBUFFER, gIBLFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gIBLRBO);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, IRR_SIZE, IRR_SIZE);

    proj = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 viewsIrr[6] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0))
    };

    // 4) ������� gEnvCubemap �� gIrradianceMap
    irradianceProg.Use();
    irradianceProg.SetInt("uEnvCubemap", 0);
    irradianceProg.SetMat4("uProjection", proj);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);

    // ״̬���ڱ���Ⱦ����� LEQUAL
    GLboolean prevCullIrr = glIsEnabled(GL_CULL_FACE);
    GLint prevCullModeIrr = GL_BACK;
    if (prevCullIrr) glGetIntegerv(GL_CULL_FACE_MODE, &prevCullModeIrr);
    GLboolean prevDepthIrr = glIsEnabled(GL_DEPTH_TEST);
    GLint prevDepthFuncIrr = GL_LESS; glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFuncIrr);

    glDisable(GL_CULL_FACE);            // �� glEnable(GL_CULL_FACE); glCullFace(GL_FRONT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    glViewport(0, 0, IRR_SIZE, IRR_SIZE);
    for (int i = 0; i < 6; ++i) {
        irradianceProg.SetMat4("uView", viewsIrr[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, gIrradianceMap, 0);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        RenderUnitCube();               // draw count ������ 36
    }

    // ��ԭ״̬ & FBO
    if (prevCullIrr) { glEnable(GL_CULL_FACE); glCullFace(prevCullModeIrr); }
    else { glDisable(GL_CULL_FACE); }
    if (prevDepthIrr) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(prevDepthFuncIrr);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "[IBL] Irradiance map generated\n";

    const int PREFILTER_SIZE = 128;  // �� 128/256
    prefilterProg.Load("shaders/prefilter_env.vert", "shaders/prefilter_env.frag");

    // Ŀ�� cubemap���� mip ����
    glGenTextures(1, &gPrefilterMap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gPrefilterMap);
    for (int i = 0; i < 6; ++i) {
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB16F,
            PREFILTER_SIZE, PREFILTER_SIZE, 0, GL_RGB, GL_FLOAT, nullptr);
    }
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glGenerateMipmap(GL_TEXTURE_CUBE_MAP);

    // ���� capture FBO/RBO
    glBindFramebuffer(GL_FRAMEBUFFER, gIBLFBO);
    glBindRenderbuffer(GL_RENDERBUFFER, gIBLRBO);

    glm::mat4 projPref = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 10.0f);
    glm::mat4 viewsPref[6] = {
        glm::lookAt(glm::vec3(0), glm::vec3(1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(-1, 0, 0), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0,-1, 0), glm::vec3(0, 0,-1)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0, 1), glm::vec3(0,-1, 0)),
        glm::lookAt(glm::vec3(0), glm::vec3(0, 0,-1), glm::vec3(0,-1, 0))
    };

    prefilterProg.Use();
    prefilterProg.SetInt("uEnvCubemap", 0);
    prefilterProg.SetMat4("uProjection", projPref);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);

    // ���沢����״̬���ڱ���Ⱦ��
    GLboolean prevCullPref = glIsEnabled(GL_CULL_FACE);
    GLint     prevCullModePref = GL_BACK; if (prevCullPref) glGetIntegerv(GL_CULL_FACE_MODE, &prevCullModePref);
    GLboolean prevDepthPref = glIsEnabled(GL_DEPTH_TEST);
    GLint     prevDepthFuncPref = GL_LESS; glGetIntegerv(GL_DEPTH_FUNC, &prevDepthFuncPref);

    glDisable(GL_CULL_FACE);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    gPrefilterMaxMip = (int)std::floor(std::log2((double)PREFILTER_SIZE));
    for (int mip = 0; mip <= gPrefilterMaxMip; ++mip) {
        int mipW = (int)(PREFILTER_SIZE * std::pow(0.5, mip));
        int mipH = mipW;
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, mipW, mipH);
        glViewport(0, 0, mipW, mipH);

        float roughness = (float)mip / (float)gPrefilterMaxMip;
        prefilterProg.SetFloat("uRoughness", roughness);

        for (int face = 0; face < 6; ++face) {
            prefilterProg.SetMat4("uView", viewsPref[face]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
                GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, gPrefilterMap, mip);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            RenderUnitCube(); // index count = 36
        }
    }

    // �ָ�״̬
    if (prevCullPref) { glEnable(GL_CULL_FACE); glCullFace(prevCullModePref); }
    else glDisable(GL_CULL_FACE);
    if (prevDepthPref) glEnable(GL_DEPTH_TEST); else glDisable(GL_DEPTH_TEST);
    glDepthFunc(prevDepthFuncPref);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    std::cout << "[IBL] Prefiltered specular env map generated (" << (gPrefilterMaxMip + 1) << " mips)\n";

    brdfLUTProg_.Load("shaders/brdf_lut.vert", "shaders/brdf_lut.frag");
    GenerateBRDFLUT(512);
    std::cout << "[IBL] BRDF LUT generated\n";

    // ȷ�� PBR shader �� BRDF LUT �������������ã��� End() ��󶨵ĵ�Ԫһ�£�
    pbrProg_.Use();
    pbrProg_.SetInt("uBRDFLUT", 12);

    initHDR(fbw, fbh);
    post_ = std::make_unique<PostProcessor>(fbw, fbh);
    post_->setToneMapParams(1.6f, 0, true);
    post_->setBloom(true, 1.2f, 0.3f, 5, 0.4f, 1);          // ���� Bloom����ֵ/����/����/ǿ��/������
}



void Renderer::Begin(const glm::mat4& view, const glm::mat4& projection) {
    PROFILE_BEGIN("GBuffer");
    // --- 1) ��¼�������/���λ�� ---
    currentView_ = view;
    currentProj_ = projection;
    currentViewInv_ = glm::inverse(view);
    camPosWS_ = glm::vec3(currentViewInv_[3]);

    // --- 2) �� GBuffer ���뼸�ν׶� ---
    gbuf_.bindForGeometryPass();

    // �����ӿڵ� GBuffer �ߴ�
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbw, &fbh);
    glViewport(0, 0, fbw, fbh);

    // --- 3) ����ĳ�ʼ��Ⱦ״̬ ---
    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LESS);
    glDisable(GL_CULL_FACE);           // �ȹص��޳�������ģ���淭���¡���ƬԪ��

    // --- 4) �𸽼������ؼ���gPosition.a = 0 ��Ϊ���޼��Ρ����֣� ---
    // ע�⣺�������������� GBuffer ����ʱ�ĸ���˳��һ��
    const float clearPos[4] = { 0, 0, 0, 0 };   // a=0 �� �޼��Σ��� Tonemap ����ʹ�ã�
    const float clearNR[4] = { 0, 0, 1, 1 };   // normal=(0,0,1), rough=1
    const float clearAlbSpec[4] = { 0, 0, 0, 0 };
    const float clearAlb[4] = { 0, 0, 0, 1 };
    const float clearNR2[4] = { 0, 0, 1, 1 };
    const float clearMetalAO[4] = { 0, 1, 0, 0 };   // AO = 1
    const float clearEmissive[4] = { 0, 0, 0, 0 };

    glClearBufferfv(GL_COLOR, 0, clearPos);      // gPosition
    glClearBufferfv(GL_COLOR, 1, clearNR);       // gNormal (legacy)
    glClearBufferfv(GL_COLOR, 2, clearAlbSpec);  // gAlbedoSpec (legacy)
    glClearBufferfv(GL_COLOR, 3, clearAlb);      // uGAlbedo
    glClearBufferfv(GL_COLOR, 4, clearNR2);      // uGNormalRough
    glClearBufferfv(GL_COLOR, 5, clearMetalAO);  // uGMetalAO
    glClearBufferfv(GL_COLOR, 6, clearEmissive); // uGEmissive

    glClearDepth(1.0);                            // �Է�����Ĺ�
    glClear(GL_DEPTH_BUFFER_BIT);

    // --- 5)����ѡ������ʵ�������ԣ�����ŵ� Init��ֻҪ VAO ����Ͳ���ÿ֡�� ---
    glBindVertexArray(cubeMesh.GetVAO());
    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    const std::size_t v4 = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * v4));
        glVertexAttribDivisor(2 + i, 1);
    }

    // --- 6) ѡ����ȷ�ļ���д�� shader��PBR �� GBuffer���������û��� uniform ---
    gbufProg_.Use();
    gbufProg_.SetMat4("uView", view);
    gbufProg_.SetMat4("uProj", projection);
    // Ĭ�ϲ��ʣ�Submit/Draw ʱ�ᱻ���ǾͲ��÷����ģ�
    gbufProg_.SetVec3("uAlbedo", glm::vec3(1.0f, 0.7f, 0.3f));
    gbufProg_.SetFloat("uRough", 0.5f);
    gbufProg_.SetFloat("uMetal", 0.0f);
    gbufProg_.SetFloat("uAO", 1.0f);
    gbufProg_.SetVec3("uEmissive", glm::vec3(0.0f));
}


void Renderer::Submit(const glm::mat4& model, const MaterialPBR& m) {
    gbufProg_.Use();
    gbufProg_.SetMat4("uModel", model);
    gbufProg_.SetMat4("uView", currentView_);
    gbufProg_.SetMat4("uProj", currentProj_);
    gbufProg_.SetVec3("uAlbedo", m.albedo);
    gbufProg_.SetFloat("uRough", m.rough);
    gbufProg_.SetFloat("uMetal", m.metal);
    gbufProg_.SetFloat("uAO", m.ao);
    gbufProg_.SetVec3("uEmissive", m.emissive);

    // --- �ֲ�״̬��ȷ����һ����д�� GBuffer ---
    GLboolean prevCull = glIsEnabled(GL_CULL_FACE);
    if (prevCull) glDisable(GL_CULL_FACE);

    // ��һ�����ɼ����ļ��Σ����÷��飩
    RenderUnitCube();

    if (prevCull) glEnable(GL_CULL_FACE);
}

void Renderer::Submit(const glm::mat4& model) {
    modelMatrices.push_back(model);
}

void Renderer::End() {
    // ͳһ��һ�γߴ�
    int fbw = 0, fbh = 0;
    glfwGetFramebufferSize(glfwGetCurrentContext(), &fbw, &fbh);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, fbw, fbh);

    const bool previewSky = (gSkyboxPreviewMode != 0);

    if (!previewSky) {
        // === 1) �ȰѼ��ε���ȴ� GBuffer ������Ĭ��֡���� ===
        gbuf_.blitDepthToDefault(fbw, fbh);

        // === 2) ����պ���Ϊ����������û�м��εĵط����֣� ===
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(GL_LEQUAL);
        glDisable(GL_CULL_FACE);
        skyboxProg.Use();
        skyboxProg.SetMat4("uView", glm::mat4(glm::mat3(currentView_)));
        skyboxProg.SetMat4("uProj", currentProj_);
        skyboxProg.SetInt("uEnvCubemap", 0);
        skyboxProg.SetFloat("uSkyExposure", 1.0f);
        skyboxProg.SetInt("uSkyMode", 1);     // 1=ACES
        skyboxProg.SetInt("uSkyDoGamma", 1);  // ���� Gamma
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);
        RenderUnitCube(); // ���ڱ�������
        glDisable(GL_DEPTH_TEST);
        // glDepthFunc(GL_LESS); // ���������Ҫ���ɻָ�
        PROFILE_END("GBuffer");

        // === 3) PBR �ϳ� �� д�� HDR FBO ===

        {
            {
                PROFILE_CPU_GPU("PBR-Compose");
                ensureHDRSize(fbw, fbh);

                glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO_);
                glViewport(0, 0, hdrW_, hdrH_);
                glDisable(GL_DEPTH_TEST);
                glClearColor(0, 0, 0, 1);
                glClear(GL_COLOR_BUFFER_BIT);

                // ȫ�����ǣ������� VAO��������ֱ�ӻ�
                pbrProg_.Use();

                // �����ع��ȹ̶� 1.0����ӳ�佻�� Tonemap
                pbrProg_.SetFloat("uExposure", 1.0f);
                pbrProg_.SetInt("uDebugShow", gDebugShowAlbedo ? 1 : 0);

                glActiveTexture(GL_TEXTURE0);  glBindTexture(GL_TEXTURE_2D, gbuf_.getPosition());
                glActiveTexture(GL_TEXTURE1);  glBindTexture(GL_TEXTURE_2D, gbuf_.getNormalRough());
                glActiveTexture(GL_TEXTURE2);  glBindTexture(GL_TEXTURE_2D, gbuf_.getAlbedo());
                glActiveTexture(GL_TEXTURE3);  glBindTexture(GL_TEXTURE_2D, gbuf_.getMetalAO());
                glActiveTexture(GL_TEXTURE4);  glBindTexture(GL_TEXTURE_2D, gbuf_.getEmissive());

                glActiveTexture(GL_TEXTURE10); glBindTexture(GL_TEXTURE_CUBE_MAP, gIrradianceMap);
                glActiveTexture(GL_TEXTURE11); glBindTexture(GL_TEXTURE_CUBE_MAP, gPrefilterMap);
                pbrProg_.SetFloat("uPrefilterMaxMip", (float)gPrefilterMaxMip);
                pbrProg_.SetInt("uBRDFLUT", 12);
                glActiveTexture(GL_TEXTURE12); glBindTexture(GL_TEXTURE_2D, gBRDFLUT_);

                glBindVertexArray(screenVAO_);
                glDrawArrays(GL_TRIANGLES, 0, 3);
            }
            
            glBindVertexArray(0);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            post_->run(hdrColor_, gbuf_.getPosition(), fbw, fbh, screenVAO_);
        }
        
    }
    else {
        // Ԥ��ģʽ��ֻ����պУ�env �� irr���������ϳ�
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);

        skyboxProg.Use();
        skyboxProg.SetMat4("uView", glm::mat4(glm::mat3(currentView_)));
        skyboxProg.SetMat4("uProj", currentProj_);
        skyboxProg.SetInt("uEnvCubemap", 0);

        glActiveTexture(GL_TEXTURE0);
        if (gSkyboxPreviewMode == 2 && gIrradianceMap)
            glBindTexture(GL_TEXTURE_CUBE_MAP, gIrradianceMap);
        else
            glBindTexture(GL_TEXTURE_CUBE_MAP, gEnvCubemap);

        RenderUnitCube();
    }
}


void Renderer::GenerateBRDFLUT(int size) {
    // ���� 2D ���� (RG16F)
    glGenTextures(1, &gBRDFLUT_);
    glBindTexture(GL_TEXTURE_2D, gBRDFLUT_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, size, size, 0, GL_RG, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // ��ʱ FBO ��Ⱦ����������
    GLuint fbo = 0, rbo = 0;
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gBRDFLUT_, 0);

    glViewport(0, 0, size, size);
    glDisable(GL_DEPTH_TEST);
    brdfLUTProg_.Use();
    brdfLUTProg_.SetVec2("uLUTResolution", glm::vec2(size, size));
    glClear(GL_COLOR_BUFFER_BIT);

    if (screenVAO_ == 0) glGenVertexArrays(1, &screenVAO_);
    glBindVertexArray(screenVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 3); // ȫ������
    glBindVertexArray(0);

    // ��ԭ
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Renderer::initHDR(int w, int h) {
    hdrW_ = w; hdrH_ = h;
    glGenFramebuffers(1, &hdrFBO_);
    glBindFramebuffer(GL_FRAMEBUFFER, hdrFBO_);
    glGenTextures(1, &hdrColor_);
    glBindTexture(GL_TEXTURE_2D, hdrColor_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, w, h, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, hdrColor_, 0);
    GLenum bufs[1] = { GL_COLOR_ATTACHMENT0 };
    glDrawBuffers(1, bufs);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
void Renderer::destroyHDR() {
    if (hdrColor_) glDeleteTextures(1, &hdrColor_), hdrColor_ = 0;
    if (hdrFBO_)   glDeleteFramebuffers(1, &hdrFBO_), hdrFBO_ = 0;
    hdrW_ = hdrH_ = 0;
}
void Renderer::ensureHDRSize(int w, int h) {
    if (w == hdrW_ && h == hdrH_) return;
    destroyHDR(); initHDR(w, h);
    if (post_) post_->resize(w, h);
}

void Renderer::createDefaultTextures() {
    // 1) sRGB �ף�Albedo fallback��
    glGenTextures(1, &defaultWhiteSRGB_);
    glBindTexture(GL_TEXTURE_2D, defaultWhiteSRGB_);
    unsigned char white[4] = { 255,255,255,255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8_ALPHA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, white);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 2) ���Ժڣ�Emissive fallback��
    glGenTextures(1, &defaultBlack_);
    glBindTexture(GL_TEXTURE_2D, defaultBlack_);
    unsigned char black[4] = { 0,0,0,255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, black);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 3) ����(0,0,1)��Normal fallback�����ԣ�
    glGenTextures(1, &defaultNormal_);
    glBindTexture(GL_TEXTURE_2D, defaultNormal_);
    unsigned char nrm[3] = { 128,128,255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, nrm);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 4) MRAO��M=0, R=1, AO=1�����ԣ�
    glGenTextures(1, &defaultMRAO_);
    glBindTexture(GL_TEXTURE_2D, defaultMRAO_);
    unsigned char mrao[3] = { 0,255,255 };
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB8, 1, 1, 0, GL_RGB, GL_UNSIGNED_BYTE, mrao);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindTexture(GL_TEXTURE_2D, 0);
}

void Renderer::bindMaterialForGBuffer(const PBRMaterial& m) {
    gbufProg_.Use();

    // ����������������ͼ����Ч������ͼʱ��ΪĬ��/���䣩
    gbufProg_.SetVec3("uAlbedo", m.albedo);
    gbufProg_.SetFloat("uRough", m.roughness);
    gbufProg_.SetFloat("uMetal", m.metallic);
    gbufProg_.SetFloat("uAO", m.ao);
    gbufProg_.SetVec3("uEmissive", m.emissive);

    // ����
    gbufProg_.SetInt("uHasAlbedoMap", m.albedoTex ? 1 : 0);
    gbufProg_.SetInt("uHasNormalMap", m.normalTex ? 1 : 0);
    gbufProg_.SetInt("uHasMRAOMap", m.mraoTex ? 1 : 0);
    gbufProg_.SetInt("uHasEmissiveMap", m.emissiveTex ? 1 : 0);

    // ����ͼ������󶨶��ף�
    glActiveTexture(GL_TEXTURE0 + TEX_ALBEDO);
    glBindTexture(GL_TEXTURE_2D, m.albedoTex ? m.albedoTex : defaultWhiteSRGB_);

    glActiveTexture(GL_TEXTURE0 + TEX_NORMAL);
    glBindTexture(GL_TEXTURE_2D, m.normalTex ? m.normalTex : defaultNormal_);

    glActiveTexture(GL_TEXTURE0 + TEX_MRAO);
    glBindTexture(GL_TEXTURE_2D, m.mraoTex ? m.mraoTex : defaultMRAO_);

    glActiveTexture(GL_TEXTURE0 + TEX_EMISSIVE);
    glBindTexture(GL_TEXTURE_2D, m.emissiveTex ? m.emissiveTex : defaultBlack_);
}


void Renderer::DrawScene(SceneNode* root) {
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    modelMatrices.clear();
    std::stack<SceneNode*> st; st.push(root);
    while (!st.empty()) {
        auto* node = st.top(); st.pop();
        modelMatrices.push_back(node->transform.GetWorldMatrix());
        for (SceneNode* ch : node->GetChildren()) st.push(ch);
    }

    glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
    glBufferData(GL_ARRAY_BUFFER,
        modelMatrices.size() * sizeof(glm::mat4),
        modelMatrices.data(),
        GL_DYNAMIC_DRAW);

    glBindVertexArray(cubeMesh.GetVAO());

    const std::size_t v4 = sizeof(glm::vec4);
    for (int i = 0; i < 4; ++i) {
        glEnableVertexAttribArray(2 + i);
        glVertexAttribPointer(2 + i, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(i * v4));
        glVertexAttribDivisor(2 + i, 1);
    }

    shader.Use();
    glm::mat4 view = glm::lookAt(glm::vec3(30, 30, 30), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));

    int fbw = 0, fbh = 0;
    GLFWwindow* cur = glfwGetCurrentContext();
    glfwGetFramebufferSize(cur, &fbw, &fbh);
    float aspect = (fbh > 0) ? (float)fbw / (float)fbh : 1.0f;
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), aspect, 0.1f, 100.f);

    shader.SetMat4("view", view);
    shader.SetMat4("projection", proj);

    glDrawArraysInstanced(GL_TRIANGLES, 0, cubeMesh.GetVertexCount(),
        static_cast<GLsizei>(modelMatrices.size()));
    glBindVertexArray(0);
}