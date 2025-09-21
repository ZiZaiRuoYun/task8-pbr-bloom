#version 330 core
in vec2 vUV;
out vec3 FragColor;

uniform sampler2D uImage;
uniform vec2 uTexelSize;   // 1/width, 1/height
uniform int uHorizontal;   // 1=Ë®Æ½, 0=ÊúÖ±

void main(){
    float w[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec2 dir = (uHorizontal==1) ? vec2(1.0, 0.0) : vec2(0.0, 1.0);

    vec3 col = texture(uImage, vUV).rgb * w[0];
    for (int i=1; i<5; ++i){
        vec2 off = dir * uTexelSize * float(i);
        col += texture(uImage, vUV + off).rgb * w[i];
        col += texture(uImage, vUV - off).rgb * w[i];
    }
    FragColor = col;
}
