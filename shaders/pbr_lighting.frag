#version 330 core
layout(location = 0) out vec4 FragColor;

in vec2 vUV;

// ===== GBuffer =====
uniform sampler2D uGPosition;     // xyz=world pos, a=mask(1=有几何,0=无)
uniform sampler2D uGNormalRough;  // rgb=normal(线性[-1,1]或已线性存储), a=roughness
uniform sampler2D uGAlbedo;       // rgb=albedo(线性!)
uniform sampler2D uGMetalAO;      // r=metallic, g=ao
uniform sampler2D uGEmissive;     // rgb=emissive(线性)

// ===== IBL =====
uniform samplerCube uIrradianceMap;
uniform samplerCube uPrefilterMap;
uniform sampler2D   uBRDFLUT;
uniform float       uPrefilterMaxMip;

// ===== Camera / Debug =====
uniform vec3 uCameraWS;
// 0 = 正常 PBR；1 = 直出 Albedo
uniform int  uDebugShow = 0;

const float PI = 3.14159265359;

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);
}

void main()
{
    // 几何遮罩
    vec4 posA = texture(uGPosition, vUV);
    if (posA.a <= 0.0) discard;

    vec3 P         = posA.xyz;
    vec4 nr        = texture(uGNormalRough, vUV);
    vec3 N         = normalize(nr.rgb);               // 已存为线性向量就直接归一化
    float rough    = clamp(nr.a, 0.04, 1.0);

    // ★★ 这里不要再 pow(...,2.2) 了：GBuffer 的 Albedo 已是线性
    vec3 albedo    = texture(uGAlbedo, vUV).rgb;
    vec2 mAO       = texture(uGMetalAO, vUV).rg;
    float metallic = clamp(mAO.r, 0.0, 1.0);
    float ao       = clamp(mAO.g, 0.0, 1.0);
    vec3 emissive  = texture(uGEmissive, vUV).rgb;

    if (uDebugShow == 1) { FragColor = vec4(albedo, 1.0); return; }

    vec3 V    = normalize(uCameraWS - P);
    float NoV = max(dot(N, V), 0.0);

    // F0：介质 0.04 与金属 Albedo 插值
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    // ===== Diffuse IBL =====
    vec3 irradiance = texture(uIrradianceMap, N).rgb;
    vec3 kS = fresnelSchlickRoughness(NoV, F0, rough);
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);
    vec3 diffuseIBL = irradiance * albedo * kD * ao;

    // ===== Specular IBL (split-sum) =====
    vec3 R = reflect(-V, N);
    float mip = rough * uPrefilterMaxMip;
    vec3 prefiltered = textureLod(uPrefilterMap, R, mip).rgb;
    vec2 brdf = texture(uBRDFLUT, vec2(NoV, rough)).rg;
    vec3 specularIBL = prefiltered * (F0 * brdf.x + brdf.y) * ao;

    // 直射光，如有再加到 directDiffuse/directSpecular。此处先置 0：
    vec3 directDiffuse = vec3(0.0);

    // ===== 合成 —— 保持在线性 HDR 域！=====
    vec3 color = diffuseIBL + specularIBL + directDiffuse + emissive;

    // 不要在这里做 Tonemap / Gamma！交给后面的 tonemap.frag。
    FragColor = vec4(color, 1.0);
}
