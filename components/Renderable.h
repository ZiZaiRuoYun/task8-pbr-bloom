#pragma once
#include <cstdint>

struct Renderable {
    std::uint32_t meshId{ 0 };
    std::uint32_t shaderId{ 0 };
    bool instanced{ false };
};
