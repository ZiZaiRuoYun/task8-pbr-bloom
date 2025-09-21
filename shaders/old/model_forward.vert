#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;   // ��ʱ�õ�һ MVP
void main() { gl_Position = uMVP * vec4(aPos, 1.0); }
