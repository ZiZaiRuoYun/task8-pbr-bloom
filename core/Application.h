#pragma once                    // �����ظ�����ͷ�ļ�
#include "Window.h"
#include "SceneNode.h"
#include "Renderer.h"           // ������Ⱦ���ӿ�
#include "Registry.h"           // ����
#include "RenderSystem.h"       // ����
#include "CameraSystem.h"       // ����

#include "assets/AssetManager.h"
#include "assets/Model.h"
#include "Shader.h"

class Application {
public:
    void Run();                 // ��ѭ������
    Registry reg;
    RenderSystem renderSys;
    CameraSystem  cameraSys;
private:
    Window window;              // ��װ GLFW �Ĵ�����
    SceneNode* root;            // ����ͼ�ĸ��ڵ�ָ��
    Renderer renderer;          // ������Ⱦ��ʵ��

    assets::Handle<assets::Model> heavyModel_;
    Shader shaderForward;
};
