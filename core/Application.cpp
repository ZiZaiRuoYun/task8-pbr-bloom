#include "Application.h"
#include "Window.h"
#include "Renderer.h"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <chrono>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include "ecs/Registry.h"
#include "systems/RenderSystem.h"
#include "systems/CameraSystem.h"
#include "components/Transform.h"
#include "components/Renderable.h"
#include "components/Camera.h"
#include "components/InputTag.h"
#include "components/MaterialPBR.h"

// Mesh IDs�����㹤�����Լ����0=Cube, 1=Plane, 2=Sphere��
constexpr std::uint32_t kCubeMesh = 0u;
constexpr std::uint32_t kPlaneMesh = 1u;
constexpr std::uint32_t kSphereMesh = 2u;

// ������Ⱦ��/ȫ�ֵĵ��Կ��أ����У�
extern bool gShowIrradiance;
extern int  gSkyboxPreviewMode;  // 0=off, 1=env, 2=irr
extern bool gDebugShowAlbedo;

namespace {
    // �� eye/target �� pitch/yaw��X=Pitch, Y=Yaw, Z=Roll=0��
    inline glm::vec3 LookAtEuler(const glm::vec3& eye, const glm::vec3& target) {
        glm::vec3 f = glm::normalize(target - eye);
        // yaw �� Y��z ǰ��x ��
        float yaw = std::atan2(f.x, f.z);
        // pitch �� X��y �ϣ�OpenGL �����Ͽ�Ϊ����/���ǵĶ�����ʵ�ֶ���
        // ������Ŀ�����Ϸ�ʱ pitch>0�����·�ʱ pitch<0
        float pitch = std::asin(f.y);
        return glm::vec3(pitch, yaw, 0.0f);
    }

    // �ɼ�����������루WASD/Space/Ctrl + ��꣩
    InputState MakeInputState(Window& window, float dt,
        double& lastX, double& lastY,
        bool& firstMouse, bool captureMouse)
    {
        GLFWwindow* w = window.GetGLFWwindow();

        double cx = 0.0, cy = 0.0;
        glfwGetCursorPos(w, &cx, &cy);
        if (firstMouse) { lastX = cx; lastY = cy; firstMouse = false; }

        float dx = 0.f, dy = 0.f;
        if (captureMouse) { dx = static_cast<float>(cx - lastX); dy = static_cast<float>(cy - lastY); }
        lastX = cx; lastY = cy;

        int fbw = 0, fbh = 0; glfwGetFramebufferSize(w, &fbw, &fbh);

        InputState in{};
        in.w = glfwGetKey(w, GLFW_KEY_W) == GLFW_PRESS;
        in.a = glfwGetKey(w, GLFW_KEY_A) == GLFW_PRESS;
        in.s = glfwGetKey(w, GLFW_KEY_S) == GLFW_PRESS;
        in.d = glfwGetKey(w, GLFW_KEY_D) == GLFW_PRESS;
        in.up = glfwGetKey(w, GLFW_KEY_SPACE) == GLFW_PRESS;
        in.down = glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
        in.mouseDX = dx; in.mouseDY = dy;
        in.fbw = fbw; in.fbh = fbh; in.dt = dt;
        return in;
    }
} // namespace

void Application::Run() {
    // ��������Ⱦ��
    window.Init(1280, 720, "Task");
    renderer.Init();

    // ===== �Ķ� 1������ʱ��������꣨�� F1 ���л������� =====
    glfwSetInputMode(window.GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window.GetGLFWwindow(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    bool  captureMouse = false;   // <== ��ʼ������
    bool  f1Held = false;
    bool  firstMouse = true;
    double lastX = 0.0, lastY = 0.0;

    // ECS
    Registry reg;
    RenderSystem renderSys; renderSys.init(&renderer);
    CameraSystem cameraSys;

    // ���ʵ��
    const Entity cam = reg.create();
    reg.add<Transform>(cam);
    reg.add<Camera>(cam, Camera{ 45.f, 0.1f, 200.f, 16.f / 9.f });
    reg.add<InputTag>(cam);

    auto& tCam = reg.get<Transform>(cam);
    // �ȸ�����ʱλ�ˣ��Ժ����ݷ��ⷽ����������
    tCam.SetLocalPosition(glm::vec3(0.f, 2.f, 8.f));
    tCam.SetEulerRotation(glm::vec3(0.f));

    // ��ݹ��죺����һ���� PBR ���ʵ�ʵ��
    auto makeEntity = [&](const glm::vec3& pos,
        const glm::vec3& scale,
        const glm::vec3& eulerRad,
        std::uint32_t meshId,
        const MaterialPBR& m) -> Entity
        {
            Entity e = reg.create();
            auto& t = reg.add<Transform>(e);
            t.SetLocalPosition(pos);
            t.SetScale(scale);
            t.SetEulerRotation(eulerRad);
            reg.add<Renderable>(e, Renderable{ meshId, 0u, false });
            reg.add<MaterialPBR>(e, m);
            return e;
        };

    // ����
    {
        MaterialPBR ground{};
        ground.albedo = glm::vec3(0.50f);
        ground.rough = 0.60f;
        ground.metal = 0.0f;
        ground.ao = 1.0f;
        ground.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(50.f, 0.5f, 50.f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            kPlaneMesh, ground);
    }

    // һ�ŶԱȷ���
    const float R = 1.0f, gap = 3.0f, y = R;

    // 1) ���ϣ��ǽ������ʹֲڣ�
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.80f, 0.12f, 0.12f);
        m.rough = 0.15f; m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(-gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 2) ���ʣ��ǽ������ߴֲڣ�
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.90f);
        m.rough = 0.80f; m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(0.0f, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 3) ����������������ʹֲڣ�
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.95f);
        m.rough = 0.06f; m.metal = 1.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(+gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 4) ��˿�������еȴֲڣ�
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(1.00f, 0.86f, 0.57f);
        m.rough = 0.35f; m.metal = 1.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(+2.f * gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }

    // 5) ���ⷽ�飨���ڹ۲� Bloom��
    Entity emissiveBox;
    glm::vec3 emissivePos{};
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(1.0f);      // ֻ������
        m.rough = 0.4f;  m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(8.0f, 7.0f, 6.0f);      // ��ʼ��Щ����
        emissivePos = glm::vec3(-2.f * gap, y, -2.f);
        emissiveBox = makeEntity(emissivePos,
            glm::vec3(R, R, R),
            glm::vec3(0.0f),
            kCubeMesh, m);
    }

    // ===== �Ķ� 2����ʼ��ʱ�Ͷ��ŷ��ⷽ�� =====
    {
        // ������ŵ�Ŀ��ǰ�Ϸ�һ�㣨��Ҳ�ɸĳɱ�� offset��
        glm::vec3 eye = emissivePos + glm::vec3(0.0f, 2.5f, 8.0f);
        glm::vec3 eul = LookAtEuler(eye, emissivePos);
        tCam.SetLocalPosition(eye);
        tCam.SetEulerRotation(eul);
    }

    // FPS ͳ��
    double lastTime = glfwGetTime();
    int    fpsFrames = 0; double fpsAccum = 0.0;
    double fpsMinMS = 1e9, fpsMaxMS = 0.0;
    const  double FPS_UPDATE_SEC = 1.0;

    // ��ѭ��
    while (!window.ShouldClose()) {
        // dt
        double now = glfwGetTime();
        float  dt = static_cast<float>(now - lastTime);
        lastTime = now;

        // FPS
        fpsFrames++; fpsAccum += dt;
        double ms = dt * 1000.0;
        fpsMinMS = std::min(fpsMinMS, ms);
        fpsMaxMS = std::max(fpsMaxMS, ms);
        if (fpsAccum >= FPS_UPDATE_SEC) {
            double fps = fpsFrames / fpsAccum;
            double avgMS = (fpsAccum / fpsFrames) * 1000.0;
            std::ostringstream oss;
            oss << std::fixed
                << "Task8 | FPS: " << std::setprecision(1) << fps
                << " | " << std::setprecision(2) << avgMS << " ms"
                << " (min " << std::setprecision(2) << fpsMinMS
                << " / max " << std::setprecision(2) << fpsMaxMS << " ms)";
            glfwSetWindowTitle(window.GetGLFWwindow(), oss.str().c_str());
            fpsFrames = 0; fpsAccum = 0.0; fpsMinMS = 1e9; fpsMaxMS = 0.0;
        }

        // �¼�
        window.PollEvents();
        GLFWwindow* w = window.GetGLFWwindow();

        // ===== �Ķ� 3��F1 �л���겶�� =====
        int f1 = glfwGetKey(w, GLFW_KEY_F1);
        if (f1 == GLFW_PRESS && !f1Held) {
            captureMouse = !captureMouse;
            glfwSetInputMode(w, GLFW_CURSOR, captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(w, GLFW_RAW_MOUSE_MOTION, captureMouse ? GLFW_TRUE : GLFW_FALSE);
            firstMouse = true; // �л�������һ֡����
        }
        f1Held = (f1 == GLFW_PRESS);

        // 5����պ�Ԥ����OFF / ENV / IRR��
        static bool key5Held = false; int k5 = glfwGetKey(w, GLFW_KEY_5);
        if (k5 == GLFW_PRESS && !key5Held) {
            gSkyboxPreviewMode = (gSkyboxPreviewMode + 1) % 3;
            gShowIrradiance = (gSkyboxPreviewMode == 2);
            if (gSkyboxPreviewMode == 0) std::cout << "[DBG] Skybox preview OFF\n";
            else if (gSkyboxPreviewMode == 1) std::cout << "[DBG] Previewing EnvCubemap (original)\n";
            else std::cout << "[DBG] Previewing IrradianceMap (blurred)\n";
        }
        key5Held = (k5 == GLFW_PRESS);

        // 6��ֱ�� GBuffer Albedo ����
        static bool key6Held = false; int k6 = glfwGetKey(w, GLFW_KEY_6);
        if (k6 == GLFW_PRESS && !key6Held) {
            gDebugShowAlbedo = !gDebugShowAlbedo;
            std::cout << "[DBG] Debug Albedo = " << (gDebugShowAlbedo ? "ON" : "OFF") << "\n";
        }
        key6Held = (k6 == GLFW_PRESS);

        // 1/2/3/4�������ⷽ��ķ���ǿ�ȣ�0 / 3 / 6 / 10��
        auto& emiMat = reg.get<MaterialPBR>(emissiveBox);
        if (glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS) emiMat.emissive = glm::vec3(0.0f);
        if (glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS) emiMat.emissive = glm::vec3(3.0f);
        if (glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS) emiMat.emissive = glm::vec3(6.0f);
        if (glfwGetKey(w, GLFW_KEY_4) == GLFW_PRESS) emiMat.emissive = glm::vec3(10.0f);

        // �������/����
        InputState in = MakeInputState(window, dt, lastX, lastY, firstMouse, captureMouse);
        cameraSys.update(reg, in);

        // �������
        CameraMatrices camM{};
        auto& tc = reg.get<Transform>(cam);
        auto& cc = reg.get<Camera>(cam);
        tc.UpdateWorldTransform();
        camM.view = glm::inverse(tc.GetWorldMatrix());
        camM.proj = cc.proj();

        // ��Ⱦ
        renderSys.update(reg, camM);

        // ��������
        glfwSwapBuffers(window.GetGLFWwindow());
    }

    window.Terminate();
}
