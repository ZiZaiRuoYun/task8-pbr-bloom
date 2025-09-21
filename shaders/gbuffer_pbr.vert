#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aTangent;

out VS_OUT {
    vec3 WorldPos;
    vec3 Normal;
    vec2 TexCoord;
    mat3 TBN;
} vs_out;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProj;

void main() {
    vec4 wp = uModel * vec4(aPos, 1.0);
    vec3 N  = normalize(mat3(transpose(inverse(uModel))) * aNormal);

    // 构建 TBN（若 aTangent 全 0，则给单位阵，FS 会走顶点法线）
    vec3 T  = mat3(uModel) * aTangent;
    bool hasTangent = length(T) > 1e-4;
    vec3 B = hasTangent ? normalize(cross(N, normalize(T))) : vec3(0,1,0);
    T      = hasTangent ? normalize(T) : vec3(1,0,0);
    mat3 TBN = mat3(T, B, N);

    vs_out.WorldPos = wp.xyz;
    vs_out.Normal   = N;
    vs_out.TexCoord = aTexCoord;
    vs_out.TBN      = hasTangent ? TBN : mat3(1.0);

    gl_Position = uProj * uView * wp;
}
