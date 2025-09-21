#version 330 core
in vec2 uv;
out vec4 FragColor;

uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uGAlbedoSpec;

uniform vec3  uCamPos;
uniform int   uLightCount;

uniform float uAmbient;          // ������ǿ��
uniform float uRadiusScale;      // �뾶��������
uniform float uIntensityScale;   // ǿ����������
uniform float uSpecPowMin;       // �߹�ֲ�����
uniform float uSpecPowMax;       // �߹���������
uniform float uExposure;         // ACES �ع�
uniform int   uUseGamma;         // 1=Gamma(2.2)
uniform int   uUseACES;          // 1=ACES�����ȼ����� Gamma��

// ����������ҹɫ
uniform float uFogDensity;       // ���� 0.015~0.04
uniform vec3  uFogColor;         // ������ vec3(0.02,0.02,0.05)
uniform int   uUseFog;           // 0=�أ�Ĭ�ϣ���1=��

const int MAX_LIGHTS = 256;

// UBO���� C++ �� std140 UBO ����
layout(std140) uniform Lights {
    vec4 pos_radius[MAX_LIGHTS];      // xyz=position, w=radius
    vec4 color_intensity[MAX_LIGHTS]; // xyz=color,    w=intensity
};

vec3 BRDF_BlinnPhong(vec3 N, vec3 V, vec3 L, vec3 albedo, float specPow, float specIntensity) {
    float NdotL = max(dot(N, L), 0.0);
    vec3  diff  = albedo * NdotL;

    vec3  H     = normalize(V + L);
    float NdotH = max(dot(N, H), 0.0);
    float spec  = pow(NdotH, specPow) * specIntensity;

    return diff + vec3(spec);
}

// ���� ACES Filmic Tone Mapping�������� HDR ӳ�䵽 LDR��
vec3 ACES(vec3 x, float exposure) {
    x *= exposure; // �ع�
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x*(a*x + b)) / (x*(c*x + d) + e), 0.0, 1.0);
}

void main() {
    vec3  worldPos = texture(uGPosition,   uv).xyz;
    vec3  N        = normalize(texture(uGNormal,   uv).xyz);
    vec4  alSpec   = texture(uGAlbedoSpec, uv);
    vec3  albedo   = alSpec.rgb;
    float specStr  = alSpec.a;

    // �����Թ⡱�Ĳ��ʣ�����ƫ��һ�㣩
    albedo *= 0.85;

    float specPow  = mix(uSpecPowMin, uSpecPowMax, clamp(specStr, 0.0, 1.0));
    vec3  V        = normalize(uCamPos - worldPos);

    vec3 colorAcc = vec3(0.0);

    // �ۼӵ��
    for (int i = 0; i < uLightCount; ++i) {
        vec3  Lpos   = pos_radius[i].xyz;
        float radius = pos_radius[i].w * uRadiusScale;
        vec3  Lcol   = color_intensity[i].xyz;
        float Lint   = color_intensity[i].w * uIntensityScale;

        vec3  Ldir = Lpos - worldPos;
        float dist = length(Ldir);
        if (dist > radius) continue;

        vec3  L = Ldir / max(dist, 1e-4);

        // ƽ�����ÿ��ľ���˥��
        float x   = dist / radius;
        float att = 1.0 / (1.0 + 4.5*x + 7.5*x*x);

        vec3 brdf = BRDF_BlinnPhong(N, V, L, albedo, specPow, specStr);
        colorAcc += brdf * Lcol * Lint * att;
    }

    // �����⣨���ⴿ�ڣ�
    vec3 outColor = albedo * uAmbient + colorAcc;

    // ��ȡ��ӵ�˥�����õ�����в�Σ�N��Y Խ��Խ����
    float ground = clamp(dot(N, vec3(0,1,0)), 0.0, 1.0);
    outColor *= mix(0.88, 1.0, ground);

    // �������Կռ���������ȥɫ��ӳ��/٤��
    float camDist = length(uCamPos - worldPos);
    float fogFac  = clamp(exp(-uFogDensity * camDist * camDist), 0.0, 1.0);
    if (uUseFog == 0) fogFac = 1.0;
    outColor = mix(uFogColor, outColor, fogFac);

    // ɫ��ӳ�� / Gamma������֮��
    if (uUseACES == 1) {
        outColor = ACES(outColor, uExposure);
    } else if (uUseGamma == 1) {
        outColor = pow(outColor, vec3(1.0/2.2));
    }

    FragColor = vec4(outColor, 1.0);
}
