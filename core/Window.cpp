#include "Window.h"
#include<iostream>

void Window::Init(int width, int height, const char* title) {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return;
    }

    // 记录初始宽高
    width_ = width;
    height_ = height;

    // 初始 framebuffer 尺寸
    glfwGetFramebufferSize(window, &fb_width_, &fb_height_);  

    glfwMakeContextCurrent(window);  // 设置 OpenGL 上下文为当前窗口

    // 把 this 存到 user pointer，回调里才能取回 Window*
    glfwSetWindowUserPointer(window, this);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return;
    }
}

void Window::PollEvents() {
    glfwPollEvents();                // 查询并处理用户输入事件
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window);  // 是否点击了关闭按钮
}

void Window::Terminate() {
    glfwDestroyWindow(window);
    glfwTerminate();                      // 清理 GLFW 全局资源
}

GLFWwindow* Window::GetGLFWwindow() const {
    return window;
}

// 从 GLFWwindow* 取回 Window*
Window* Window::FromGLFW(GLFWwindow* win) {
    return static_cast<Window*>(glfwGetWindowUserPointer(win));
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    if (width <= 0 || height <= 0) return; // 最小化时保护
    glViewport(0, 0, width, height);

    if (auto* self = Window::FromGLFW(window)) {
        self->fb_width_ = width;
        self->fb_height_ = height;

        // 同步一下 window 逻辑尺寸（可选）
        glfwGetWindowSize(window, &self->width_, &self->height_);
        // 如果你的投影矩阵保存在 Window/Camera 里，这里顺手更新一下也行：
        // self->projection_ = glm::perspective(glm::radians(self->fov_), float(width)/float(height), 0.1f, 100.0f);
    }
}
