#version 330 core
out vec2 vUV;

void main() {
    // ȫ�����ǣ���ʡһ�ζ��㻺�棩
    // gl_VertexID: 0,1,2 �� 3 �������Ƕ���
    vec2 pos = vec2(
        (gl_VertexID == 2) ?  3.0 : -1.0,
        (gl_VertexID == 1) ?  3.0 : -1.0
    );
    vUV = 0.5 * (pos + 1.0);   // ӳ�䵽 [0,1]
    gl_Position = vec4(pos, 0.0, 1.0);
}
