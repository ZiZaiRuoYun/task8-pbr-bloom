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

    // ����ͬʱӵ�� Ts... ��ʵ��
    // ʵ�֣��� Ts... �ġ���һ�����͡���Ӧ�ĳ�Ϊ����������������� has/get ���
    template<typename... Ts, typename Fn>
    void view(Fn&& fn) {
        using First = std::tuple_element_t<0, std::tuple<Ts...>>;
        auto& outer = pool<First>();                        // ����û���������ؿճأ���ȫ
        for (Entity e : outer.entities()) {
            if ((has<Ts>(e) && ...)) {                      // ȫ�����ڲŻص�
                fn(e, get<Ts>(e)...);
            }
        }
    }

private:
    // ---- ���Ͳ���������ع��� ----
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
