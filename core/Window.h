#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>

class Window {
public:
    void Init(int width, int height, const char* title); // 初始化 GLFW 窗口
    void PollEvents();            // 处理事件
    bool ShouldClose() const;     // 检查窗口是否关闭
    void Terminate();             // 释放 GLFW 资源
    GLFWwindow* GetGLFWwindow() const;
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);

    // 通过 GLFWwindow* 还原到 Window*（从 user pointer 取回）
    static Window* FromGLFW(GLFWwindow* window);

    // 读当前宽高
    int width() const { return width_; }
    int height() const { return height_; }

    // 便捷获取当前绘制用的宽高与纵横比（用 framebuffer 尺寸更稳）
    int fbWidth()  const { return fb_width_; }
    int fbHeight() const { return fb_height_; }
    float aspect()  const { return fb_height_ > 0 ? float(fb_width_) / float(fb_height_) : 1.0f; }
private:
    GLFWwindow* window = nullptr; // GLFW 提供的窗口句柄
    int width_ = 0, height_ = 0;     // window size（逻辑像素）
    int fb_width_ = 0, fb_height_ = 0; // framebuffer size（物理像素）
};
