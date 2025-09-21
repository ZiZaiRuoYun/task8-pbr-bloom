#version 330 core
out vec4 FragColor;
uniform samplerCube uEnvCubemap;
uniform vec3 uDir;              // 采样方向（比如 +X = (1,0,0)）
void main(){
    FragColor = textureLod(uEnvCubemap, normalize(uDir), 0.0);
}
