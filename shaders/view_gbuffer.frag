#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uGPosition;
uniform sampler2D uGNormalRough;
uniform sampler2D uGAlbedo;
uniform sampler2D uGMetalAO;
uniform sampler2D uGEmissive;

// ºÊ»›æ…Õº
uniform sampler2D uGNormalLegacy;
uniform sampler2D uGAlbedoSpec;

uniform int mode = 0;
// 0:Albedo 1:Normal 2:Position 3:Roughness 4:Metallic 5:AO 6:Emissive
// 7:Legacy AlbedoSpec.rgb  8:Legacy a(rough/spec)

void main() {
    vec2 uv = clamp(vUV, 0.0, 1.0);

    if (mode == 0) {
        FragColor = vec4(texture(uGAlbedo, uv).rgb, 1.0);
    } else if (mode == 1) {
        vec3 N = normalize(texture(uGNormalRough, uv).rgb);
        FragColor = vec4(0.5*N + 0.5, 1.0);
    } else if (mode == 2) {
        vec3 P = texture(uGPosition, uv).rgb;
        FragColor = vec4(0.5 + 0.05*P, 1.0);
    } else if (mode == 3) {
        float r = texture(uGNormalRough, uv).a;
        FragColor = vec4(r, r, r, 1.0);
    } else if (mode == 4) {
        float m = texture(uGMetalAO, uv).r;
        FragColor = vec4(vec3(m), 1.0);
    } else if (mode == 5) {
        float ao = texture(uGMetalAO, uv).g;
        FragColor = vec4(vec3(ao), 1.0);
    } else if (mode == 6) {
        FragColor = vec4(texture(uGEmissive, uv).rgb, 1.0);
    } else if (mode == 7) {
        FragColor = vec4(texture(uGAlbedoSpec, uv).rgb, 1.0);
    } else if (mode == 8) {
        float a = texture(uGAlbedoSpec, uv).a;
        FragColor = vec4(vec3(a), 1.0);
    } else {
        FragColor = vec4(texture(uGAlbedo, uv).rgb, 1.0);
    }
}
