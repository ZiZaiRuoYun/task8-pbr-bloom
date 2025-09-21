#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct Camera {
    float fovDeg{ 45.f };
    float znear{ 0.1f }, zfar{ 100.f };
    float aspect{ 16.f / 9.f };

    glm::mat4 proj() const {
        return glm::perspective(glm::radians(fovDeg), aspect, znear, zfar);
    }
};
