#pragma once
#include <atomic>

namespace assets {

    enum class ResourceState { Loading, Ready, Failed };

    struct Resource {
        std::atomic<ResourceState> state{ ResourceState::Loading };
        virtual ~Resource() = default;
    };

} // namespace assets
