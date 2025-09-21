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

    // ��¼��ʼ���
    width_ = width;
    height_ = height;

    // ��ʼ framebuffer �ߴ�
    glfwGetFramebufferSize(window, &fb_width_, &fb_height_);  

    glfwMakeContextCurrent(window);  // ���� OpenGL ������Ϊ��ǰ����

    // �� this �浽 user pointer���ص������ȡ�� Window*
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
    glfwPollEvents();                // ��ѯ�������û������¼�
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window);  // �Ƿ����˹رհ�ť
}

void Window::Terminate() {
    glfwDestroyWindow(window);
    glfwTerminate();                      // ���� GLFW ȫ����Դ
}

GLFWwindow* Window::GetGLFWwindow() const {
    return window;
}

// �� GLFWwindow* ȡ�� Window*
Window* Window::FromGLFW(GLFWwindow* win) {
    return static_cast<Window*>(glfwGetWindowUserPointer(win));
}

void Window::framebuffer_size_callback(GLFWwindow* window, int width, int height) 
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    if (width <= 0 || height <= 0) return; // ��С��ʱ����
    glViewport(0, 0, width, height);

    if (auto* self = Window::FromGLFW(window)) {
        self->fb_width_ = width;
        self->fb_height_ = height;

        // ͬ��һ�� window �߼��ߴ磨��ѡ��
        glfwGetWindowSize(window, &self->width_, &self->height_);
        // ������ͶӰ���󱣴��� Window/Camera �����˳�ָ���һ��Ҳ�У�
        // self->projection_ = glm::perspective(glm::radians(self->fov_), float(width)/float(height), 0.1f, 100.0f);
    }
}
