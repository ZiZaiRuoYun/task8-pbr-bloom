#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 2) in mat4 instanceModel;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
}