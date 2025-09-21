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

// Mesh IDs（按你工程里的约定：0=Cube, 1=Plane, 2=Sphere）
constexpr std::uint32_t kCubeMesh = 0u;
constexpr std::uint32_t kPlaneMesh = 1u;
constexpr std::uint32_t kSphereMesh = 2u;

// 来自渲染器/全局的调试开关（已有）
extern bool gShowIrradiance;
extern int  gSkyboxPreviewMode;  // 0=off, 1=env, 2=irr
extern bool gDebugShowAlbedo;

namespace {
    // 由 eye/target 推 pitch/yaw（X=Pitch, Y=Yaw, Z=Roll=0）
    inline glm::vec3 LookAtEuler(const glm::vec3& eye, const glm::vec3& target) {
        glm::vec3 f = glm::normalize(target - eye);
        // yaw 绕 Y：z 前、x 右
        float yaw = std::atan2(f.x, f.z);
        // pitch 绕 X：y 上，OpenGL 里向上看为正角/负角的定义因实现而异
        // 这里让目标在上方时 pitch>0，在下方时 pitch<0
        float pitch = std::asin(f.y);
        return glm::vec3(pitch, yaw, 0.0f);
    }

    // 采集飞行相机输入（WASD/Space/Ctrl + 鼠标）
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
    // 窗口与渲染器
    window.Init(1280, 720, "Task");
    renderer.Init();

    // ===== 改动 1：启动时不捕获鼠标（按 F1 才切换到捕获） =====
    glfwSetInputMode(window.GetGLFWwindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    if (glfwRawMouseMotionSupported())
        glfwSetInputMode(window.GetGLFWwindow(), GLFW_RAW_MOUSE_MOTION, GLFW_FALSE);

    bool  captureMouse = false;   // <== 初始不捕获
    bool  f1Held = false;
    bool  firstMouse = true;
    double lastX = 0.0, lastY = 0.0;

    // ECS
    Registry reg;
    RenderSystem renderSys; renderSys.init(&renderer);
    CameraSystem cameraSys;

    // 相机实体
    const Entity cam = reg.create();
    reg.add<Transform>(cam);
    reg.add<Camera>(cam, Camera{ 45.f, 0.1f, 200.f, 16.f / 9.f });
    reg.add<InputTag>(cam);

    auto& tCam = reg.get<Transform>(cam);
    // 先给个临时位姿，稍后依据发光方块重新设置
    tCam.SetLocalPosition(glm::vec3(0.f, 2.f, 8.f));
    tCam.SetEulerRotation(glm::vec3(0.f));

    // 便捷构造：生成一个带 PBR 材质的实体
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

    // 地面
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

    // 一排对比方块
    const float R = 1.0f, gap = 3.0f, y = R;

    // 1) 塑料（非金属，低粗糙）
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.80f, 0.12f, 0.12f);
        m.rough = 0.15f; m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(-gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 2) 介质（非金属，高粗糙）
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.90f);
        m.rough = 0.80f; m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(0.0f, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 3) 镜面金属（金属，低粗糙）
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(0.95f);
        m.rough = 0.06f; m.metal = 1.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(+gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }
    // 4) 拉丝金属（中等粗糙）
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(1.00f, 0.86f, 0.57f);
        m.rough = 0.35f; m.metal = 1.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(0.0f);
        makeEntity(glm::vec3(+2.f * gap, y, 0.0f), glm::vec3(R, R, R), glm::vec3(0.0f), kCubeMesh, m);
    }

    // 5) 发光方块（用于观察 Bloom）
    Entity emissiveBox;
    glm::vec3 emissivePos{};
    {
        MaterialPBR m{};
        m.albedo = glm::vec3(1.0f);      // 只看发光
        m.rough = 0.4f;  m.metal = 0.0f; m.ao = 1.0f;
        m.emissive = glm::vec3(8.0f, 7.0f, 6.0f);      // 初始有些亮度
        emissivePos = glm::vec3(-2.f * gap, y, -2.f);
        emissiveBox = makeEntity(emissivePos,
            glm::vec3(R, R, R),
            glm::vec3(0.0f),
            kCubeMesh, m);
    }

    // ===== 改动 2：初始化时就盯着发光方块 =====
    {
        // 把相机放到目标前上方一点（你也可改成别的 offset）
        glm::vec3 eye = emissivePos + glm::vec3(0.0f, 2.5f, 8.0f);
        glm::vec3 eul = LookAtEuler(eye, emissivePos);
        tCam.SetLocalPosition(eye);
        tCam.SetEulerRotation(eul);
    }

    // FPS 统计
    double lastTime = glfwGetTime();
    int    fpsFrames = 0; double fpsAccum = 0.0;
    double fpsMinMS = 1e9, fpsMaxMS = 0.0;
    const  double FPS_UPDATE_SEC = 1.0;

    // 主循环
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

        // 事件
        window.PollEvents();
        GLFWwindow* w = window.GetGLFWwindow();

        // ===== 改动 3：F1 切换鼠标捕获 =====
        int f1 = glfwGetKey(w, GLFW_KEY_F1);
        if (f1 == GLFW_PRESS && !f1Held) {
            captureMouse = !captureMouse;
            glfwSetInputMode(w, GLFW_CURSOR, captureMouse ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
            if (glfwRawMouseMotionSupported())
                glfwSetInputMode(w, GLFW_RAW_MOUSE_MOTION, captureMouse ? GLFW_TRUE : GLFW_FALSE);
            firstMouse = true; // 切换后避免第一帧大跳
        }
        f1Held = (f1 == GLFW_PRESS);

        // 5：天空盒预览（OFF / ENV / IRR）
        static bool key5Held = false; int k5 = glfwGetKey(w, GLFW_KEY_5);
        if (k5 == GLFW_PRESS && !key5Held) {
            gSkyboxPreviewMode = (gSkyboxPreviewMode + 1) % 3;
            gShowIrradiance = (gSkyboxPreviewMode == 2);
            if (gSkyboxPreviewMode == 0) std::cout << "[DBG] Skybox preview OFF\n";
            else if (gSkyboxPreviewMode == 1) std::cout << "[DBG] Previewing EnvCubemap (original)\n";
            else std::cout << "[DBG] Previewing IrradianceMap (blurred)\n";
        }
        key5Held = (k5 == GLFW_PRESS);

        // 6：直出 GBuffer Albedo 调试
        static bool key6Held = false; int k6 = glfwGetKey(w, GLFW_KEY_6);
        if (k6 == GLFW_PRESS && !key6Held) {
            gDebugShowAlbedo = !gDebugShowAlbedo;
            std::cout << "[DBG] Debug Albedo = " << (gDebugShowAlbedo ? "ON" : "OFF") << "\n";
        }
        key6Held = (k6 == GLFW_PRESS);

        // 1/2/3/4：调发光方块的发光强度（0 / 3 / 6 / 10）
        auto& emiMat = reg.get<MaterialPBR>(emissiveBox);
        if (glfwGetKey(w, GLFW_KEY_1) == GLFW_PRESS) emiMat.emissive = glm::vec3(0.0f);
        if (glfwGetKey(w, GLFW_KEY_2) == GLFW_PRESS) emiMat.emissive = glm::vec3(3.0f);
        if (glfwGetKey(w, GLFW_KEY_3) == GLFW_PRESS) emiMat.emissive = glm::vec3(6.0f);
        if (glfwGetKey(w, GLFW_KEY_4) == GLFW_PRESS) emiMat.emissive = glm::vec3(10.0f);

        // 相机输入/更新
        InputState in = MakeInputState(window, dt, lastX, lastY, firstMouse, captureMouse);
        cameraSys.update(reg, in);

        // 相机矩阵
        CameraMatrices camM{};
        auto& tc = reg.get<Transform>(cam);
        auto& cc = reg.get<Camera>(cam);
        tc.UpdateWorldTransform();
        camM.view = glm::inverse(tc.GetWorldMatrix());
        camM.proj = cc.proj();

        // 渲染
        renderSys.update(reg, camM);

        // 交换缓冲
        glfwSwapBuffers(window.GetGLFWwindow());
    }

    window.Terminate();
}
