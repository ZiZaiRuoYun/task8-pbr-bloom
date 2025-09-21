#version 330 core
in vec3 vLocalPos;
out vec4 FragColor;

uniform sampler2D uEquirect;                   // 输入的HDR长方形纹理

const vec2 INV_ATAN = vec2(0.1591, 0.3183);    // 1/(2π), 1/π

vec2 sampleSphericalMap(vec3 v) {
    vec2 uv = vec2(atan(v.z, v.x), asin(v.y));
    uv *= INV_ATAN;                             // [-π..π]x[-π/2..π/2] → [-0.5..0.5]x[-0.5..0.5]
    uv += 0.5;                                  // → [0..1]
    return uv;
}

void main() {
    vec3 n  = normalize(vLocalPos);
    vec2 uv = sampleSphericalMap(n);
    vec3 col = texture(uEquirect, uv).rgb;
    FragColor = vec4(col, 1.0);
}
