#version 330 core
in vec2 vUV;
out vec3 FragColor;

uniform sampler2D uHDR;
uniform float uThreshold;   // 1.0~3.0 建议
uniform float uKnee;        // 0~1，软阈值

void main(){
    vec3 c = texture(uHDR, vUV).rgb;

    // soft knee（近似实现）
    float t = uThreshold;
    float k = t * uKnee + 1e-4;
    vec3  x = max(c - vec3(t - k), 0.0);
    vec3  soft = (x * x) / (vec3(k) * vec3(k) + 1e-4);

    vec3  hard = max(c - vec3(t), 0.0);
    vec3  bright = max(hard, soft);

    FragColor = bright;
}
