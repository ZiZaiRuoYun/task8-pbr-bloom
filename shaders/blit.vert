#version 330 core
out vec2 vUV;

void main() {
    // 全屏三角（更省一次顶点缓存）
    // gl_VertexID: 0,1,2 → 3 个大三角顶点
    vec2 pos = vec2(
        (gl_VertexID == 2) ?  3.0 : -1.0,
        (gl_VertexID == 1) ?  3.0 : -1.0
    );
    vUV = 0.5 * (pos + 1.0);   // 映射到 [0,1]
    gl_Position = vec4(pos, 0.0, 1.0);
}
