#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
    // === 旧接口：保持函数名与签名不变 ===
    void SetScale(const glm::vec3& s) { scale_ = s; dirty_ = true; }                // 设置本地缩放
    glm::vec3 GetScale() const { return scale_; }                             // 获取当前缩放

    void SetLocalPosition(const glm::vec3& pos) { position_ = pos; dirty_ = true; }           // 设置本地位置
    void Rotate(const glm::vec3& angles) { eulerRad_ += angles; dirty_ = true; }       // 增量欧拉旋转（弧度）
    void SetEulerRotation(const glm::vec3& ang) { eulerRad_ = ang; dirty_ = true; }          // 设置绝对欧拉角（弧度）

    void SetParent(Transform* p) { parent_ = p; dirty_ = true; }               // 设置父变换（可为 nullptr）

    // 计算世界变换矩阵（若无父节点则等于本地矩阵）
    void UpdateWorldTransform() { updateMatrices_(); }

    glm::vec3 GetWorldPosition() const {                                             // 世界位置向量
        const_cast<Transform*>(this)->updateMatrices_();
        glm::vec4 wp = worldMatrix_ * glm::vec4(0, 0, 0, 1);
        return glm::vec3(wp);
    }

    glm::mat4 GetWorldMatrix() const {                                               // 世界变换矩阵
        const_cast<Transform*>(this)->updateMatrices_();
        return worldMatrix_;
    }

    // === 新接口：如需直接拿本地/世界矩阵也支持 ===
    glm::mat4 LocalMatrix() const { const_cast<Transform*>(this)->updateLocal_(); return localMatrix_; }

    // 方便 ECS/Renderer 使用的统一名字
    glm::mat4 matrix() const { return GetWorldMatrix(); }

    // 直接访问本地状态的简便 getter（可选）
    const glm::vec3& position()   const { return position_; }
    const glm::vec3& rotationRad()const { return eulerRad_; }
    const glm::vec3& scale()      const { return scale_; }

private:
    // ------ 本地状态 ------
    glm::vec3 position_{ 0.0f, 0.0f, 0.0f };  // 本地坐标
    glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };     // 本地缩放
    glm::vec3 eulerRad_{ 0.0f, 0.0f, 0.0f };  // XYZ 欧拉角（单位：弧度）

    Transform* parent_{ nullptr };            // 父节点指针（可空）
    // ------ 缓存矩阵 ------
    mutable bool  dirty_{ true };
    mutable glm::mat4 localMatrix_{ 1.0f };
    mutable glm::mat4 worldMatrix_{ 1.0f };

    void updateLocal_() const {
        if (!dirty_) return;
        glm::mat4 m(1.0f);
        m = glm::translate(m, position_);
        // 顺序保持你原项目的 XYZ（pitch=x, yaw=y, roll=z），如需改顺序按渲染约定调整
        m = glm::rotate(m, eulerRad_.y, glm::vec3{ 0,1,0 }); // yaw
        m = glm::rotate(m, eulerRad_.x, glm::vec3{ 1,0,0 }); // pitch
        m = glm::rotate(m, eulerRad_.z, glm::vec3{ 0,0,1 }); // roll
        m = glm::scale(m, scale_);
        localMatrix_ = m;
        dirty_ = false;
    }

    void updateMatrices_() const {
        updateLocal_();
        if (parent_) {
            worldMatrix_ = parent_->GetWorldMatrix() * localMatrix_;
        }
        else {
            worldMatrix_ = localMatrix_;
        }
    }
};
