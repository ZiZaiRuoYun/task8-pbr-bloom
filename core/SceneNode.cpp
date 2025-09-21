// SceneNode.cpp
#include "SceneNode.h"

SceneNode::SceneNode(const std::string& name) : name(name) {}

void SceneNode::AddChild(SceneNode* child) {
    child->parent = this;
    child->transform.SetParent(&this->transform);
    children.push_back(child);
}

void SceneNode::Update() {
    transform.UpdateWorldTransform();
    for (auto child : children) {
        child->Update();
    }
}

