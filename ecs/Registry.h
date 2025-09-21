#pragma once
#include "Entity.h"
#include "Component.h"
#include <memory>
#include <typeindex>
#include <unordered_map>
#include <tuple>   

class Registry {
public:
    Entity create() { return em_.create(); }
    void destroy(Entity e) { em_.destroy(e); }

    template<typename T, typename... Args>
    T& add(Entity e, Args&&... args) {
        return pool<T>().add(e, std::forward<Args>(args)...);
    }

    template<typename T>
    bool has(Entity e) const {
        auto key = std::type_index(typeid(T));
        auto it = pools_.find(key);
        if (it == pools_.end()) return false;
        return static_cast<const ErasedPoolImpl<T>*>(it->second.get())->pool.has(e);
    }

    template<typename T>
    T& get(Entity e) {
        return pool<T>().get(e);
    }

    template<typename T>
    void remove(Entity e) {
        pool<T>().remove(e);
    }

    // 遍历同时拥有 Ts... 的实体
    // 实现：以 Ts... 的“第一个类型”对应的池为外层迭代，其他组件用 has/get 检查
    template<typename... Ts, typename Fn>
    void view(Fn&& fn) {
        using First = std::tuple_element_t<0, std::tuple<Ts...>>;
        auto& outer = pool<First>();                        // 若还没创建，返回空池，安全
        for (Entity e : outer.entities()) {
            if ((has<Ts>(e) && ...)) {                      // 全部存在才回调
                fn(e, get<Ts>(e)...);
            }
        }
    }

private:
    // ---- 类型擦除的组件池管理 ----
    struct ErasedPool { virtual ~ErasedPool() = default; };

    template<typename T>
    struct ErasedPoolImpl : ErasedPool {
        ComponentPool<T> pool;
    };

    template<typename T>
    ComponentPool<T>& pool() {
        auto key = std::type_index(typeid(T));
        auto it = pools_.find(key);
        if (it == pools_.end()) {
            auto p = std::make_unique<ErasedPoolImpl<T>>();
            auto* raw = p.get();
            pools_.emplace(key, std::move(p));
            return raw->pool;
        }
        return static_cast<ErasedPoolImpl<T>*>(it->second.get())->pool;
    }

    template<typename T>
    const ComponentPool<T>& cpool() const {
        auto key = std::type_index(typeid(T));
        return static_cast<const ErasedPoolImpl<T>*>(pools_.at(key).get())->pool;
    }

    EntityManager em_;
    std::unordered_map<std::type_index, std::unique_ptr<ErasedPool>> pools_;
};
