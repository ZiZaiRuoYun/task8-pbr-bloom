#version 330 core
layout (location = 0) out vec4 gPosition;      // legacy: world pos (a=mask)
layout (location = 1) out vec4 gNormal;        // legacy: normal (a 未用)
layout (location = 2) out vec4 gAlbedoSpec;    // legacy: rgb=albedo, a=spec (这里放 roughness 或 metallic 近似)

layout (location = 3) out vec4 uGAlbedo;       // PBR: rgb=albedo, a 保留
layout (location = 4) out vec4 uGNormalRough;  // PBR: xyz=normal, a=roughness
layout (location = 5) out vec2 uGMetalAO;      // PBR: r=metallic, g=ao
layout (location = 6) out vec3 uGEmissive;     // PBR: emissive

in VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
} fs_in;

// —— 常量参数（当没有贴图时使用）——
uniform vec3  uAlbedo   = vec3(1.0, 0.7, 0.3);
uniform float uRough    = 0.5;
uniform float uMetal    = 0.0;
uniform float uAO       = 1.0;
uniform vec3  uEmissive = vec3(0.0);

// —— 贴图与开关（有图时覆盖常量）——
// 注意：Albedo 贴图推荐用 sRGB 格式创建（GL_SRGB8_ALPHA8），采样会自动转线性
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
    // 法线贴图采样（线性）—— 从 [0,1] 映射到 [-1,1]，再 TBN 到世界空间
    vec3 nTS = texture(uNormalMap, fs_in.TexCoord).xyz * 2.0 - 1.0;
    // 若顶点没切线，vs_out.TBN=I，此时等价于使用顶点法线
    return normalize(fs_in.TBN * nTS);
}

void main(){
    // 选择性取色
    vec3  albedo = (uHasAlbedoMap==1) ? pow(texture(uAlbedoMap, fs_in.TexCoord).rgb, vec3(2.2))
                                      : uAlbedo;         // sRGB→linear
    vec3  N      = sampleNormalWS();
    float metallic  = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).r : uMetal;
    float roughness = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).g : uRough;
    float ao        = (uHasMRAOMap==1) ? texture(uMetalRoughAOMap, fs_in.TexCoord).b : uAO;
    vec3  emissive  = (uHasEmissiveMap==1) ? texture(uEmissiveMap, fs_in.TexCoord).rgb : uEmissive;

    // 写旧附件（保持兼容）
    gPosition   = vec4(fs_in.WorldPos, 1.0);
    gNormal     = vec4(N, 1.0);
    gAlbedoSpec = vec4(albedo, roughness); // a 填 roughness 以兼容旧路径需求

    // 写 PBR 附件
    uGAlbedo      = vec4(albedo, 1.0);
    uGNormalRough = vec4(N, clamp(roughness, 0.04, 1.0));
    uGMetalAO     = vec2(clamp(metallic, 0.0, 1.0), clamp(ao, 0.0, 1.0));
    uGEmissive    = emissive;
}
