#version 330 core
in vec2 vUV;
out vec4 FragColor;

uniform sampler2D uHDR;        // 0: HDR 颜色
uniform sampler2D uGMask;      // 8: gPosition，a=1 有几何，a=0 无几何（天空盒）
uniform sampler2D uBloom;      // 9: 经过模糊的亮部贴图（可空）

uniform int   uUseBloom;       // 0/1
uniform float uBloomStrength;  // 0~2

uniform float uExposure;
uniform int   uMode;           // 0=Reinhard, 1=ACES
uniform int   uDoGamma;        // 0/1

vec3 ACESFitted(vec3 x) {
    const float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    // 保留已绘制的天空盒：没有几何（a<=0）的像素全部丢弃
    if (texture(uGMask, vUV).a <= 0.0)
        discard;

    vec3 hdr = texture(uHDR, vUV).rgb;

    // 只在几何上叠加 Bloom（亮部模糊）
    if (uUseBloom == 1) {
        hdr += texture(uBloom, vUV).rgb * uBloomStrength;
    }

    // 曝光
    hdr *= uExposure;

    // 色调映射
    vec3 ldr = (uMode == 0) ? hdr / (1.0 + hdr) : ACESFitted(hdr);

    // γ 矫正（屏幕是 sRGB）
    if (uDoGamma == 1)
        ldr = pow(ldr, vec3(1.0/2.2));

    FragColor = vec4(ldr, 1.0);
}
