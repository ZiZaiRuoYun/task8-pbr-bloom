#version 330 core
in vec3 vLocalPos;
out vec4 FragColor;

uniform samplerCube uEnvCubemap;
uniform float uRoughness;

// ������������Ԥ���ˣ��� 512~1024 ���У�128��128 �����ֱ���ʱ 1024 ��ƽ����
const uint SAMPLE_COUNT = 1024u;

// Hammersley ���� + GGX ��Ҫ��ӳ��
float radicalInverse_VdC(uint bits){
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}
vec2 hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}
vec3 importanceSampleGGX(vec2 Xi, vec3 N, float roughness){
    float a = roughness * roughness;
    float phi = 2.0 * 3.1415926535 * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0)*Xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta*cosTheta));
    vec3 H = vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);

    // TBN
    vec3 up = abs(N.y) < 0.999 ? vec3(0.0,1.0,0.0) : vec3(0.0,0.0,1.0);
    vec3 T  = normalize(cross(up, N));
    vec3 B  = cross(N, T);
    return normalize(T * H.x + B * H.y + N * H.z);
}

void main() {
    vec3 N = normalize(vLocalPos);
    vec3 R = N; // Ԥ����ʱ���� V��N��split-sum �еĽ��ƣ�

    vec3 prefiltered = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = importanceSampleGGX(Xi, N, uRoughness);
        vec3 L  = normalize(2.0 * dot(R,H) * H - R);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0) {
            prefiltered += texture(uEnvCubemap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }
    prefiltered = (totalWeight > 0.0) ? prefiltered / totalWeight : prefiltered;
    FragColor = vec4(prefiltered, 1.0);
}
