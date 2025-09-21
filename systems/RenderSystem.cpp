#include "RenderSystem.h"
#include "../core/Renderer.h"
#include "../components/MaterialPBR.h"
#include "profiling/Profiler.h"

void RenderSystem::update(Registry& reg, const CameraMatrices& cam) {
    PROFILE_FRAME_BEGIN();
    // 绑定 GBuffer，设置 VP 矩阵
    renderer_->Begin(cam.view, cam.proj);

    // --- 有 MaterialPBR 的实体 ---
    reg.view<Transform, Renderable, MaterialPBR>([&](Entity e,
        Transform& t,
        Renderable& r,
        MaterialPBR& m) {
            (void)r; // 目前忽略 meshId
            renderer_->Submit(t.matrix(), m);  // 使用材质版本
        });

    // --- 没有 MaterialPBR 的实体（用默认材质） ---
    reg.view<Transform, Renderable>([&](Entity e,
        Transform& t,
        Renderable& r) {
            // 如果这个实体有 MaterialPBR，就跳过（避免重复绘制）
            if (reg.has<MaterialPBR>(e)) return;
            renderer_->Submit(t.matrix());     // 默认材质版本
        });

    renderer_->End();
    PROFILE_FRAME_END();
}
