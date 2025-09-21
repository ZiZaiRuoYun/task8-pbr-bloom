#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;

// 实例化的 model 矩阵（占 4 个 attribute 槽：2,3,4,5）
layout (location=2) in mat4 iModel;

uniform mat4 uView;
uniform mat4 uProj;

out VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    flat int InstanceID;   // 把实例 ID 传下去
} vs_out;

void main() {
    vec4 wp = iModel * vec4(aPos, 1.0);
    vs_out.WorldPos = wp.xyz;

    // 法线：用 model 的逆转置上三行三列（这里 iModel 不含非均匀缩放的话直接用 mat3(iModel) 也可）
    mat3 Nmat = mat3(transpose(inverse(iModel)));
    vs_out.WorldNormal = normalize(Nmat * aNormal);

    vs_out.InstanceID = gl_InstanceID;

    gl_Position = uProj * uView * wp;
}
