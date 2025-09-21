#version 330 core
in vec3 vLocalPos;
out vec4 FragColor;

uniform samplerCube uEnvCubemap;

// 采样数（离线卷积，64~1024 都可；32x32 分辨率下 512 已很顺滑）
const uint SAMPLE_COUNT = 512u;

// 切线空间基
mat3 computeTangentBasis(vec3 N) {
    vec3 up    = abs(N.y) < 0.999 ? vec3(0.0, 1.0, 0.0) : vec3(0.0, 0.0, 1.0);
    vec3 T     = normalize(cross(up, N));
    vec3 B     = cross(N, T);
    return mat3(T, B, N);
}

// Hammersley + GGX 的低差异序列，这里用均匀半球 + 余弦权重即可
float radicalInverse_VdC(uint bits){
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // / 0x100000000
}
vec2 hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}

vec3 hemisphereCosine(vec2 Xi) {
    float phi = 2.0 * 3.1415926535 * Xi.x;
    float cosTheta = sqrt(1.0 - Xi.y);   // 余弦加权
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);
    return vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
}

void main() {
    vec3 N = normalize(vLocalPos);
    mat3 TBN = computeTangentBasis(N);

    vec3 irradiance = vec3(0.0);
    float weightSum = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 L  = hemisphereCosine(Xi);          // 切线空间半球方向
        L       = normalize(TBN * L);            // 变到世界空间

        float NdotL = max(dot(N, L), 0.0);
        irradiance += texture(uEnvCubemap, L).rgb * NdotL;
        weightSum  += NdotL;
    }
    irradiance = (weightSum > 0.0) ? irradiance / weightSum : irradiance;

    FragColor = vec4(irradiance, 1.0);
}
