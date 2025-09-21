#pragma once
#include <glm/glm.hpp>

struct MaterialPBR {
    glm::vec3 albedo = glm::vec3(0.8f);
    float     rough = 0.5f;   // 0..1
    float     metal = 0.0f;   // 0..1
    float     ao = 1.0f;   // 0..1
    glm::vec3 emissive = glm::vec3(0.0f);
};
