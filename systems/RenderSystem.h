#pragma once
#include "../ecs/Registry.h"
#include "../components/Transform.h"
#include "../components/Renderable.h"
#include "../components/MaterialPBR.h"
#include <glm/glm.hpp>

struct CameraMatrices {
    glm::mat4 view{ 1.0f };
    glm::mat4 proj{ 1.0f };
};

class Renderer; // Ç°ÖÃÉùÃ÷

class RenderSystem {
public:
    void init(Renderer* r) { renderer_ = r; }
    void update(Registry& reg, const CameraMatrices& cam);
private:
    Renderer* renderer_{ nullptr };
};
