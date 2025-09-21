#version 330 core
in vec3 vDir;
out vec4 FragColor;

uniform samplerCube uEnvCubemap;
uniform float uSkyExposure = 1.0;
uniform int   uSkyMode     = 1;   // 0=Reinhard, 1=ACES
uniform int   uSkyDoGamma  = 1;

vec3 ACESFitted(vec3 x){
    float a=2.51, b=0.03, c=2.43, d=0.59, e=0.14;
    return clamp((x*(a*x+b))/(x*(c*x+d)+e), 0.0, 1.0);
}

void main()
{
    vec3 hdr = texture(uEnvCubemap, normalize(vDir)).rgb * uSkyExposure;
    vec3 ldr = (uSkyMode==0) ? (hdr/(1.0+hdr)) : ACESFitted(hdr);
    if (uSkyDoGamma==1) ldr = pow(ldr, vec3(1.0/2.2));
    FragColor = vec4(ldr, 1.0);
}
