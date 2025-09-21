#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;   // 临时用单一 MVP
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
