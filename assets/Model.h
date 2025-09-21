#pragma once
#include <vector>
#include <cstdint>
#include <atomic>
#include <cfloat>        // FLT_MAX
#include <glm/glm.hpp>   // 需要顶点/矩阵类型
#include "Resource.h"

namespace assets {

    struct Vertex {
        glm::vec3 pos{ 0 };
        glm::vec3 normal{ 0,0,1 };
        glm::vec2 uv{ 0 };
    };

    struct SubmeshCPU {
        std::vector<Vertex> vertices;
        std::vector<std::uint32_t> indices;
    };

    struct SubmeshGPU {
        unsigned vao = 0, vbo = 0, ebo = 0;
        std::size_t indexCount = 0;
    };

    struct Model : public Resource {
        // 后台线程填充
        std::vector<SubmeshCPU> meshesCPU;

        // 主线程上传
        std::vector<SubmeshGPU> meshesGPU;
        std::atomic<bool> gpu_uploaded{ false };

        // AABB（后台解析时更新）
        glm::vec3 aabbMin{ FLT_MAX,  FLT_MAX,  FLT_MAX };
        glm::vec3 aabbMax{ -FLT_MAX, -FLT_MAX, -FLT_MAX };
    };

} // namespace assets
