#version 330 core
layout (location = 0) in vec3 aPos;
out vec3 vLocalPos;

uniform mat4 uProjection;
uniform mat4 uView;

void main() {
    vLocalPos   = aPos;                         // 立方体的本地方向
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
