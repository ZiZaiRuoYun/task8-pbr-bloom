#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window {
public:
    void Init(int width, int height, const char* title); // ��ʼ�� GLFW ����
    void PollEvents();            // �����¼�
    bool ShouldClose() const;     // ��鴰���Ƿ�ر�
    void Terminate();             // �ͷ� GLFW ��Դ
    GLFWwindow* GetGLFWwindow() const;
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // ͨ�� GLFWwindow* ��ԭ�� Window*���� user pointer ȡ�أ�
    static Window* FromGLFW(GLFWwindow* window);

    // ����ǰ���
    int width() const { return width_; }
    int height() const { return height_; }

    // ��ݻ�ȡ��ǰ�����õĿ�����ݺ�ȣ��� framebuffer �ߴ���ȣ�
    int fbWidth()  const { return fb_width_; }
    int fbHeight() const { return fb_height_; }
    float aspect()  const { return fb_height_ > 0 ? float(fb_width_) / float(fb_height_) : 1.0f; }
private:
    GLFWwindow* window = nullptr; // GLFW �ṩ�Ĵ��ھ��
    int width_ = 0, height_ = 0;     // window size���߼����أ�
    int fb_width_ = 0, fb_height_ = 0; // framebuffer size���������أ�
};
