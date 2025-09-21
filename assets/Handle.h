#pragma once
#include <memory>
#include "Resource.h"

namespace assets {

    template<typename T>
    class Handle {
    public:
        Handle() = default;

        bool Ready()  const {
            auto p = ptr_.lock();
            return p && p->state.load() == ResourceState::Ready;
        }
        bool Failed() const {
            auto p = ptr_.lock();
            return p && p->state.load() == ResourceState::Failed;
        }
        std::shared_ptr<T> Get() const {
            return std::static_pointer_cast<T>(ptr_.lock());
        }
        explicit operator bool() const { return !ptr_.expired(); }

    private:
        friend class AssetManager;
        explicit Handle(const std::shared_ptr<T>& p) : ptr_(p) {}
        std::weak_ptr<Resource> ptr_;
    };

} // namespace assets
