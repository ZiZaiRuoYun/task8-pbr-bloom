#version 330 core
in vec2 vUV;
in vec3 vWorldPos;

flat in vec3  vColor;
flat in float vIntensity;

out vec4 FragColor;

uniform vec3  uCamPos;
uniform float uFogDensity; 
uniform vec3  uFogColor;
uniform int   uUseFog;

void main() {
    // Բ�����
    vec2 p = vUV * 2.0 - 1.0;
    float r2 = dot(p, p);
    if (r2 > 1.0) discard;
    float alpha = smoothstep(1.0, 0.0, r2);

    // ��ɫ������һЩ��
    vec3 col = vColor * vIntensity * 2.0;

    // ���� lighting.frag ͬ��ʽ��
    float d   = length(uCamPos - vWorldPos);
    float fog = clamp(exp(-uFogDensity * d * d), 0.0, 1.0);
    if (uUseFog == 0) fog = 1.0;
    col = mix(uFogColor, col, fog);


    FragColor = vec4(col, alpha);
}
