#version 330 core
// ---- Legacy MRT����������/���ԣ�----
layout (location = 0) out vec4 gPosition;      // world pos
layout (location = 1) out vec4 gNormal;        // world normal (xyz), a=1
layout (location = 2) out vec4 gAlbedoSpec;    // legacy: rgb=albedo, a=roughness

// ---- PBR MRT��B �棩----
layout (location = 3) out vec4 gAlbedo;        // rgb=albedo, a=1
layout (location = 4) out vec4 gNormalRough;   // xyz=normal, a=roughness
layout (location = 5) out vec2 gMetalAO;       // r=metallic, g=ao
layout (location = 6) out vec3 gEmissive;      // rgb=emissive

in VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    flat int InstanceID;
} fs_in;

// ���� �ɵ�������������Ҳ��Ĭ��ֵ�� ����
// ������������ӳ�� Roughness ��������������ӳ�� Metallic
uniform int uGridCols = 12;  // ����ʵ����������ֵ������ȷҲ�޷���
uniform int uGridRows = 12;  // ����ʵ����
// ��ɫ�������������������ֶ��󣬲�Ӱ�� PBR �߼���
uniform vec3 uBaseColor = vec3(1.0, 0.7, 0.3);
// AO/Emissive �ȸ�����
uniform float uAO  = 1.0;
uniform vec3  uEmissive = vec3(0.0);

// clamp ����
float saturate(float x){ return clamp(x, 0.0, 1.0); }

void main() {
    vec3 N = normalize(fs_in.WorldNormal);

    // ===== gl_InstanceID �� (row, col) ӳ�� =====
    int cols = max(uGridCols, 1);
    int rows = max(uGridRows, 1);

    int col = fs_in.InstanceID % cols;
    int row = (fs_in.InstanceID / cols) % rows;

    // ��һ���� [0,1]
    float fx = (cols > 1) ? float(col) / float(cols - 1) : 0.0;
    float fy = (rows > 1) ? float(row) / float(rows - 1) : 0.0;

    // ===== ���ʹ��� =====
    // �ֲڶȣ��غ���� 0.05 �� 1.0
    float roughness = mix(0.05, 1.0, fx);

    // �����ȣ�������� 0.0 �� 1.0
    float metallic  = fy;

    // AO/Emissive �̶�
    float ao = uAO;
    vec3  emissive = uEmissive;

    // ����ѡ���� Albedo �ڲ�ͬʵ���зǳ���΢�ı仯��������������
    // ע�⣺�ⲻӰ�� PBR �߼���ֻ�ǵ��Ը�ֱ��
    vec3 albedo = uBaseColor * (0.95 + 0.05 * fract(sin(float(fs_in.InstanceID) * 12.9898) * 43758.5453));

    // ===== Legacy ���ţ����ڻ���/���ԣ� =====
    gPosition   = vec4(fs_in.WorldPos, 1.0);
    gNormal     = vec4(N, 1.0);
    gAlbedoSpec = vec4(albedo, roughness); // �ɵ� a ͨ�������� roughness

    // ===== B �� PBR ���� =====
    gAlbedo       = vec4(albedo, 1.0);
    gNormalRough  = vec4(N, roughness);
    gMetalAO      = vec2(metallic, ao);
    gEmissive     = emissive;
}
