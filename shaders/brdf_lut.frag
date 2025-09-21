#version 330 core
out vec2 FragColor;          // RG16F ´æ A,B
uniform vec2 uLUTResolution; // (size, size)

const float PI = 3.14159265359;

float GeometrySchlickGGX(float NdotV, float roughness) {
    float a = roughness;
    float k = (a * a) * 0.5;
    return NdotV / (NdotV * (1.0 - k) + k);
}
float GeometrySmith(float NdotV, float NdotL, float roughness) {
    return GeometrySchlickGGX(NdotV, roughness) * GeometrySchlickGGX(NdotL, roughness);
}
vec3 ImportanceSampleGGX(vec2 Xi, float roughness) {
    float a = roughness * roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a*a - 1.0) * Xi.y));
    float sinTheta = sqrt(max(0.0, 1.0 - cosTheta*cosTheta));
    return vec3(cos(phi)*sinTheta, sin(phi)*sinTheta, cosTheta);
}
float radicalInverse_VdC(uint bits) {
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10; // 2^-32
}
vec2 hammersley(uint i, uint N) {
    return vec2(float(i)/float(N), radicalInverse_VdC(i));
}

vec2 IntegrateBRDF(float NdotV, float rough) {
    vec3 V = vec3(sqrt(max(1.0 - NdotV*NdotV, 0.0)), 0.0, NdotV);
    float A = 0.0, B = 0.0;
    const uint SAMPLE_COUNT = 1024u;

    for (uint i=0u; i<SAMPLE_COUNT; ++i) {
        vec2 Xi = hammersley(i, SAMPLE_COUNT);
        vec3 H  = normalize(ImportanceSampleGGX(Xi, rough));
        vec3 L  = normalize(2.0 * dot(V,H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V,H), 0.0);

        if (NdotL > 0.0) {
            float G    = GeometrySmith(NdotV, NdotL, rough);
            float Gvis = (G * VdotH) / max(NdotH * NdotV, 1e-5);
            float Fc   = pow(1.0 - VdotH, 5.0);
            A += (1.0 - Fc) * Gvis;
            B += Fc * Gvis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main() {
    float NdotV = gl_FragCoord.x / uLUTResolution.x;
    float rough = gl_FragCoord.y / uLUTResolution.y;
    FragColor = IntegrateBRDF(clamp(NdotV,0.0,1.0), clamp(rough,0.0,1.0));
}
