#include "RenderSystem.h"
#include "../core/Renderer.h"
#include "../components/MaterialPBR.h"
#include "profiling/Profiler.h"

void RenderSystem::update(Registry& reg, const CameraMatrices& cam) {
    PROFILE_FRAME_BEGIN();
    // �� GBuffer������ VP ����
    renderer_->Begin(cam.view, cam.proj);

    // --- �� MaterialPBR ��ʵ�� ---
    reg.view<Transform, Renderable, MaterialPBR>([&](Entity e,
        Transform& t,
        Renderable& r,
        MaterialPBR& m) {
            (void)r; // Ŀǰ���� meshId
            renderer_->Submit(t.matrix(), m);  // ʹ�ò��ʰ汾
        });

    // --- û�� MaterialPBR ��ʵ�壨��Ĭ�ϲ��ʣ� ---
    reg.view<Transform, Renderable>([&](Entity e,
        Transform& t,
        Renderable& r) {
            // ������ʵ���� MaterialPBR���������������ظ����ƣ�
            if (reg.has<MaterialPBR>(e)) return;
            renderer_->Submit(t.matrix());     // Ĭ�ϲ��ʰ汾
        });

    renderer_->End();
    PROFILE_FRAME_END();
}
