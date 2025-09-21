#pragma once
#include <glad/glad.h>
#include <memory>
class Shader;

class BloomEffect {
public:
    bool init(int w, int h, int downscale = 2);   // 1/2 �ߴ����
    void resize(int w, int h);
    GLuint process(GLuint hdrTex, int screenW, int screenH, GLuint fsVAO);

    // ����
    float threshold = 1.2f;
    float knee = 0.5f;
    int   iterations = 6;     // ˮƽ+��ֱ һ���� 1 ��
    float strength = 0.8f;
    int   downscale = 2;

private:
    int w_ = 0, h_ = 0;
    GLuint fbo_[2] = { 0,0 };
    GLuint tex_[2] = { 0,0 };
    std::unique_ptr<Shader> extract_, blur_;
    void alloc(int w, int h);
};
