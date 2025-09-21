#include "CameraSystem.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

void CameraSystem::update(Registry& reg, const InputState& in) {
    const float move = 15.0f;
    const float sens = 0.0025f;

    reg.view<Transform, Camera, InputTag>([&](Entity, Transform& t, Camera& c, InputTag&) {
        glm::vec3 eul = t.rotationRad();       // 当前 (pitch,x) (yaw,y) (roll,z)
        eul.y -= in.mouseDX * sens;            // yaw
        eul.x -= in.mouseDY * sens;            // pitch

        // 俯仰限幅（留一点安全边界，避免右向量退化）
        const float kMaxPitch = 1.562f;        // ≈ 89.5°
        if (eul.x > kMaxPitch) eul.x = kMaxPitch;
        if (eul.x < -kMaxPitch) eul.x = -kMaxPitch;
        t.SetEulerRotation(eul);

        // 依据 Yaw→Pitch 求方向
        glm::vec3 forward{
            std::cos(eul.x) * -std::sin(eul.y),
            std::sin(eul.x),
            std::cos(eul.x) * -std::cos(eul.y)
        };
        glm::vec3 right = glm::cross(forward, { 0,1,0 });
        float rl = glm::length(right);
        if (rl > 1e-6f) right /= rl; else right = { 1,0,0 };

        // 位移
        glm::vec3 delta{ 0 };
        if (in.w) delta += forward;
        if (in.s) delta -= forward;
        if (in.d) delta += right;
        if (in.a) delta -= right;
        if (in.up)   delta += glm::vec3{ 0,1,0 };
        if (in.down) delta -= glm::vec3{ 0,1,0 };
        if (glm::length(delta) > 1e-6f) delta = glm::normalize(delta);

        glm::vec3 pos = t.position();
        pos += delta * move * in.dt;
        t.SetLocalPosition(pos);

        // 更新纵横比
        if (in.fbh > 0) c.aspect = float(in.fbw) / float(in.fbh);
        });
}
