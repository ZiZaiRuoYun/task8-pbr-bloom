#version 330 core
layout (location=0) in vec2 aPos;
out vec2 vUV;
out vec3 vWorldPos;             // 传给片元做雾

uniform mat4 uView;
uniform mat4 uProj;
uniform mat4 uViewInv;          // inverse(view)
uniform float uSize;

const int MAX_LIGHTS = 256;
layout(std140) uniform Lights {
    vec4 pos_radius[MAX_LIGHTS];
    vec4 color_intensity[MAX_LIGHTS];
};

flat out vec3  vColor;
flat out float vIntensity;

void main() {
    int i = gl_InstanceID;
    vec3 Lpos = pos_radius[i].xyz;
    vColor     = color_intensity[i].xyz;
    vIntensity = color_intensity[i].w;

    vec3 right = normalize(vec3(uViewInv[0].xyz));
    vec3 up    = normalize(vec3(uViewInv[1].xyz));

    vec3 world = Lpos + right * (aPos.x * uSize) + up * (aPos.y * uSize);
    vWorldPos  = world;

    gl_Position = uProj * uView * vec4(world, 1.0);
    vUV = aPos * 0.5 + 0.5;
}
