#version 330 core
// ---- Legacy MRT（保留兼容/调试）----
layout (location = 0) out vec4 gPosition;      // world pos
layout (location = 1) out vec4 gNormal;        // world normal (xyz), a=1
layout (location = 2) out vec4 gAlbedoSpec;    // legacy: rgb=albedo, a=roughness

// ---- PBR MRT（B 版）----
layout (location = 3) out vec4 gAlbedo;        // rgb=albedo, a=1
layout (location = 4) out vec4 gNormalRough;   // xyz=normal, a=roughness
layout (location = 5) out vec2 gMetalAO;       // r=metallic, g=ao
layout (location = 6) out vec3 gEmissive;      // rgb=emissive

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    flat int InstanceID;
} fs_in;

// —— 可调参数（不设置也有默认值） ——
// 横向列数用于映射 Roughness ；纵向行数用于映射 Metallic
uniform int uGridCols = 12;  // 横向实例数（估计值，不精确也无妨）
uniform int uGridRows = 12;  // 纵向实例数
// 颜色基调（可用来快速区分对象，不影响 PBR 逻辑）
uniform vec3 uBaseColor = vec3(1.0, 0.7, 0.3);
// AO/Emissive 先给常量
uniform float uAO  = 1.0;
uniform vec3  uEmissive = vec3(0.0);

// clamp 辅助
float saturate(float x){ return clamp(x, 0.0, 1.0); }

void main() {
    vec3 N = normalize(fs_in.WorldNormal);

    // ===== gl_InstanceID → (row, col) 映射 =====
    int cols = max(uGridCols, 1);
    int rows = max(uGridRows, 1);

    int col = fs_in.InstanceID % cols;
    int row = (fs_in.InstanceID / cols) % rows;

    // 归一化到 [0,1]
    float fx = (cols > 1) ? float(col) / float(cols - 1) : 0.0;
    float fy = (rows > 1) ? float(row) / float(rows - 1) : 0.0;

    // ===== 材质规则 =====
    // 粗糙度：沿横向从 0.05 → 1.0
    float roughness = mix(0.05, 1.0, fx);

    // 金属度：沿纵向从 0.0 → 1.0
    float metallic  = fy;

    // AO/Emissive 固定
    float ao = uAO;
    vec3  emissive = uEmissive;

    // （可选）让 Albedo 在不同实例有非常轻微的变化，便于肉眼区分
    // 注意：这不影响 PBR 逻辑，只是调试更直观
    vec3 albedo = uBaseColor * (0.95 + 0.05 * fract(sin(float(fs_in.InstanceID) * 12.9898) * 43758.5453));

    // ===== Legacy 三张（便于回退/调试） =====
    gPosition   = vec4(fs_in.WorldPos, 1.0);
    gNormal     = vec4(N, 1.0);
    gAlbedoSpec = vec4(albedo, roughness); // 旧的 a 通道继续放 roughness

    // ===== B 版 PBR 附件 =====
    gAlbedo       = vec4(albedo, 1.0);
    gNormalRough  = vec4(N, roughness);
    gMetalAO      = vec2(metallic, ao);
    gEmissive     = emissive;
}
