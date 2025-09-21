#pragma once                    // 避免重复包含头文件
#include "Window.h"
#include "SceneNode.h"
#include "Renderer.h"           // 新增渲染器接口
#include "Registry.h"           // 新增
#include "RenderSystem.h"       // 新增
#include "CameraSystem.h"       // 新增

#include "assets/AssetManager.h"
#include "assets/Model.h"
#include "Shader.h"

class Application {
public:
    void Run();                 // 主循环函数
    Registry reg;
    RenderSystem renderSys;
    CameraSystem  cameraSys;
private:
    Window window;              // 封装 GLFW 的窗口类
    SceneNode* root;            // 场景图的根节点指针
    Renderer renderer;          // 持有渲染器实例

    assets::Handle<assets::Model> heavyModel_;
    Shader shaderForward;
};
