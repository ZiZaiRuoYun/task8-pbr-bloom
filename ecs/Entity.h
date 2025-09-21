#pragma once
#include <cstdint>

using Entity = std::uint32_t;
constexpr Entity kInvalidEntity = 0;

struct EntityManager {
    Entity next{ 1 };
    Entity create() { return next++; }
    void destroy(Entity) { /* 简化：先不做复用 */ }
};
