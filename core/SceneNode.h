#pragma once
#include <vector>
#include <string>
#include "Transform.h"

class SceneNode {
public:
    SceneNode(const std::string& name);      // ���캯�������ýڵ���
    void AddChild(SceneNode* child);         // ����ӽڵ�
    void Update();                           // �ݹ����������ӽڵ�ı任
    Transform transform;                     // �ڵ�ı任���

    glm::mat4 GetWorldMatrix() const { return transform.GetWorldMatrix(); }
    const std::vector<SceneNode*>& GetChildren() const { return children; }

private:
    std::string name;                        // �ڵ���
    SceneNode* parent = nullptr;             // ���ڵ�
    std::vector<SceneNode*> children;        // �ӽڵ��б�
};
