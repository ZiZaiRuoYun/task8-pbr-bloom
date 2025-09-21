#version 330 core
layout (location=0) in vec3 aPos;
layout (location=1) in vec3 aNormal;

// ʵ������ model ����ռ 4 �� attribute �ۣ�2,3,4,5��
layout (location=2) in mat4 iModel;

uniform mat4 uView;
uniform mat4 uProj;

out VS_OUT {
    vec3 WorldPos;
    vec3 WorldNormal;
    flat int InstanceID;   // ��ʵ�� ID ����ȥ
} vs_out;

void main() {
    vec4 wp = iModel * vec4(aPos, 1.0);
    vs_out.WorldPos = wp.xyz;

    // ���ߣ��� model ����ת�����������У����� iModel �����Ǿ������ŵĻ�ֱ���� mat3(iModel) Ҳ�ɣ�
    mat3 Nmat = mat3(transpose(inverse(iModel)));
    vs_out.WorldNormal = normalize(Nmat * aNormal);

    vs_out.InstanceID = gl_InstanceID;

    gl_Position = uProj * uView * wp;
}
