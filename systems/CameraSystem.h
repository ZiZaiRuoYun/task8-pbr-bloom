#pragma once
#include "../ecs/Registry.h"
#include "../components/Transform.h"
#include "../components/Camera.h"
#include "../components/InputTag.h"

struct InputState {
    bool w{}, a{}, s{}, d{}, up{}, down{};
    float mouseDX{}, mouseDY{};
    float dt{};
    int fbw{}, fbh{}; // Ϊ�˸��� aspect
};

class CameraSystem {
public:
    void update(Registry& reg, const InputState& in);
};
