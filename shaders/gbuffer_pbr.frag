#version 330 core
layout (location = 0) out vec4 gPosition;      // legacy: world pos (a=mask)
layout (location = 1) out vec4 gNormal;        // legacy: normal (a δ��)
layout (location = 2) out vec4 gAlbedoSpec;    // legacy: rgb=albedo, a=spec (����� roughness �� metallic ����)

layout (location = 3) out vec4 uGAlbedo;       // PBR: rgb=albedo, a ����
layout (location = 4) out vec4 uGNormalRough;  // PBR: xyz=normal, a=roughness
layout (location = 5) out vec2 uGMetalAO;      // PBR: r=metallic, g=ao
layout (location = 6) out vec3 uGEmissive;     // PBR: emissive

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
} fs_in;

// ���� ������������û����ͼʱʹ�ã�����
uniform vec3  uAlbedo   = vec3(1.0, 0.7, 0.3);
uniform float uRough    = 0.5;
uniform float uMetal    = 0.0;
uniform float uAO       = 1.0;
uniform vec3  uEmissive = vec3(0.0);

// ���� ��ͼ�뿪�أ���ͼʱ���ǳ���������
// ע�⣺Albedo ��ͼ�Ƽ��� sRGB ��ʽ������GL_SRGB8_ALPHA8�����������Զ�ת����
uniform sampler2D uAlbedoMap;
uniform sampler2D uNormalMap;
uniform sampler2D uMetalRoughAOMap;  // R=Metal, G=Rough, B=AO
uniform sampler2D uEmissiveMap;

uniform int uHasAlbedoMap  = 0;
uniform int uHasNormalMap  = 0;
uniform int uHasMRAOMap    = 0;
uniform int uHasEmissiveMap= 0;

vec3 sampleNormalWS(){
    if (uHasNormalMap == 0) return normalize(fs_in.Normal);
    // ������ͼ���������ԣ����� �� [0,1] ӳ�䵽 [-1,1]���� TBN ������ռ�
    vec3 nTS = texture(uNormalMap, fs_in.TexCoord).xyz * 2.0 - 1.0;
    // ������û���ߣ�vs_out.TBN=I����ʱ�ȼ���ʹ�ö��㷨��
    return normalize(fs_in.TBN * nTS);
}

void main(){
    // ѡ����ȡɫ
    vec3  albedo = (uHasAlbedoMap==1) ? pow(texture(uAlbedoMap, fs_in.TexCoord).rgb, vec3(2.2))
                                      : uAlbedo;         // sRGB��linear
    vec3  N      = sampleNormalWS();
    float metallic  = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).r : uMetal;
    float roughness = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).g : uRough;
    float ao        = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).b : uAO;
    vec3  emissive  = (uHasEmissiveMap==1) ? texture(uEmissiveMap, fs_in.TexCoord).rgb : uEmissive;

    // д�ɸ��������ּ��ݣ�
    gPosition   = vec4(fs_in.WorldPos, 1.0);
    gNormal     = vec4(N, 1.0);
    gAlbedoSpec = vec4(albedo, roughness); // a �� roughness �Լ��ݾ�·������

    // д PBR ����
    uGAlbedo      = vec4(albedo, 1.0);
    uGNormalRough = vec4(N, clamp(roughness, 0.04, 1.0));
    uGMetalAO     = vec2(clamp(metallic, 0.0, 1.0), clamp(ao, 0.0, 1.0));
    uGEmissive    = emissive;
}
