#version 330 core
in vec2 uv;
out vec4 FragColor;

uniform sampler2D uGPosition;
uniform sampler2D uGNormal;
uniform sampler2D uGAlbedoSpec;

uniform vec3  uCamPos;
uniform int   uLightCount;

uniform float uAmbient;          // 环境光强度
uniform float uRadiusScale;      // 半径整体缩放
uniform float uIntensityScale;   // 强度整体缩放
uniform float uSpecPowMin;       // 高光粗糙下限
uniform float uSpecPowMax;       // 高光锐利上限
uniform float uExposure;         // ACES 曝光
uniform int   uUseGamma;         // 1=Gamma(2.2)
uniform int   uUseACES;          // 1=ACES（优先级高于 Gamma）

// 新增：雾与夜色
uniform float uFogDensity;       // 典型 0.015~0.04
uniform vec3  uFogColor;         // 深蓝黑 vec3(0.02,0.02,0.05)
uniform int   uUseFog;           // 0=关（默认），1=开

const int MAX_LIGHTS = 256;

// UBO：与 C++ 的 std140 UBO 对齐
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

// 近似 ACES Filmic Tone Mapping（把线性 HDR 映射到 LDR）
vec3 ACES(vec3 x, float exposure) {
    x *= exposure; // 曝光
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

    // 更“吃光”的材质（整体偏暗一点）
    albedo *= 0.85;

    float specPow  = mix(uSpecPowMin, uSpecPowMax, clamp(specStr, 0.0, 1.0));
    vec3  V        = normalize(uCamPos - worldPos);

    vec3 colorAcc = vec3(0.0);

    // 累加点光
    for (int i = 0; i < uLightCount; ++i) {
        vec3  Lpos   = pos_radius[i].xyz;
        float radius = pos_radius[i].w * uRadiusScale;
        vec3  Lcol   = color_intensity[i].xyz;
        float Lint   = color_intensity[i].w * uIntensityScale;

        vec3  Ldir = Lpos - worldPos;
        float dist = length(Ldir);
        if (dist > radius) continue;

        vec3  L = Ldir / max(dist, 1e-4);

        // 平滑、好看的距离衰减
        float x   = dist / radius;
        float att = 1.0 / (1.0 + 4.5*x + 7.5*x*x);

        vec3 brdf = BRDF_BlinnPhong(N, V, L, albedo, specPow, specStr);
        colorAcc += brdf * Lcol * Lint * att;
    }

    // 环境光（避免纯黑）
    vec3 outColor = albedo * uAmbient + colorAcc;

    // 轻度“接地衰减”让地面更有层次（N·Y 越大越亮）
    float ground = clamp(dot(N, vec3(0,1,0)), 0.0, 1.0);
    outColor *= mix(0.88, 1.0, ground);

    // 雾（在线性空间里做，再去色调映射/伽马）
    float camDist = length(uCamPos - worldPos);
    float fogFac  = clamp(exp(-uFogDensity * camDist * camDist), 0.0, 1.0);
    if (uUseFog == 0) fogFac = 1.0;
    outColor = mix(uFogColor, outColor, fogFac);

    // 色调映射 / Gamma（在雾之后）
    if (uUseACES == 1) {
        outColor = ACES(outColor, uExposure);
    } else if (uUseGamma == 1) {
        outColor = pow(outColor, vec3(1.0/2.2));
    }

    FragColor = vec4(outColor, 1.0);
}
