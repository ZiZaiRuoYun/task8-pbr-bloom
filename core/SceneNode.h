#pragma once
#include <vector>
#include <string>
#include "Transform.h"

class SceneNode {
public:
    SceneNode(const std::string& name);      // 构造函数，设置节点名
    void AddChild(SceneNode* child);         // 添加子节点
    void Update();                           // 递归更新自身和子节点的变换
    Transform transform;                     // 节点的变换组件

    glm::mat4 GetWorldMatrix() const { return transform.GetWorldMatrix(); }
    const std::vector<SceneNode*>& GetChildren() const { return children; }

private:
    std::string name;                        // 节点名
    SceneNode* parent = nullptr;             // 父节点
    std::vector<SceneNode*> children;        // 子节点列表
};
