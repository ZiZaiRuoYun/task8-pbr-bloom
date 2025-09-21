#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class Transform {
public:
    // === �ɽӿڣ����ֺ�������ǩ������ ===
    void SetScale(const glm::vec3& s) { scale_ = s; dirty_ = true; }                // ���ñ�������
    glm::vec3 GetScale() const { return scale_; }                             // ��ȡ��ǰ����

    void SetLocalPosition(const glm::vec3& pos) { position_ = pos; dirty_ = true; }           // ���ñ���λ��
    void Rotate(const glm::vec3& angles) { eulerRad_ += angles; dirty_ = true; }       // ����ŷ����ת�����ȣ�
    void SetEulerRotation(const glm::vec3& ang) { eulerRad_ = ang; dirty_ = true; }          // ���þ���ŷ���ǣ����ȣ�

    void SetParent(Transform* p) { parent_ = p; dirty_ = true; }               // ���ø��任����Ϊ nullptr��

    // ��������任�������޸��ڵ�����ڱ��ؾ���
    void UpdateWorldTransform() { updateMatrices_(); }

    glm::vec3 GetWorldPosition() const {                                             // ����λ������
        const_cast<Transform*>(this)->updateMatrices_();
        glm::vec4 wp = worldMatrix_ * glm::vec4(0, 0, 0, 1);
        return glm::vec3(wp);
    }

    glm::mat4 GetWorldMatrix() const {                                               // ����任����
        const_cast<Transform*>(this)->updateMatrices_();
        return worldMatrix_;
    }

    // === �½ӿڣ�����ֱ���ñ���/�������Ҳ֧�� ===
    glm::mat4 LocalMatrix() const { const_cast<Transform*>(this)->updateLocal_(); return localMatrix_; }

    // ���� ECS/Renderer ʹ�õ�ͳһ����
    glm::mat4 matrix() const { return GetWorldMatrix(); }

    // ֱ�ӷ��ʱ���״̬�ļ�� getter����ѡ��
    const glm::vec3& position()   const { return position_; }
    const glm::vec3& rotationRad()const { return eulerRad_; }
    const glm::vec3& scale()      const { return scale_; }

private:
    // ------ ����״̬ ------
    glm::vec3 position_{ 0.0f, 0.0f, 0.0f };  // ��������
    glm::vec3 scale_{ 1.0f, 1.0f, 1.0f };     // ��������
    glm::vec3 eulerRad_{ 0.0f, 0.0f, 0.0f };  // XYZ ŷ���ǣ���λ�����ȣ�

    Transform* parent_{ nullptr };            // ���ڵ�ָ�루�ɿգ�
    // ------ ������� ------
    mutable bool  dirty_{ true };
    mutable glm::mat4 localMatrix_{ 1.0f };
    mutable glm::mat4 worldMatrix_{ 1.0f };

    void updateLocal_() const {
        if (!dirty_) return;
        glm::mat4 m(1.0f);
        m = glm::translate(m, position_);
        // ˳�򱣳���ԭ��Ŀ�� XYZ��pitch=x, yaw=y, roll=z���������˳����ȾԼ������
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
