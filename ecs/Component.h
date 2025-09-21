#pragma once
#include <unordered_map>
#include <vector>

template<typename T>
class ComponentPool {
public:
    bool has(Entity e) const { return lookup_.count(e) != 0; }

    template<typename... Args>
    T& add(Entity e, Args&&... args) {
        if (has(e)) return get(e);
        std::size_t idx = data_.size();
        data_.emplace_back(std::forward<Args>(args)...);
        lookup_[e] = idx;
        reverse_.push_back(e);
        return data_.back();
    }

    T& get(Entity e) { return data_[lookup_.at(e)]; }
    const T& get(Entity e) const { return data_[lookup_.at(e)]; }

    void remove(Entity e) {
        auto it = lookup_.find(e);
        if (it == lookup_.end()) return;
        std::size_t idx = it->second, last = data_.size() - 1;
        if (idx != last) {
            data_[idx] = std::move(data_[last]);
            reverse_[idx] = reverse_[last];
            lookup_[reverse_[idx]] = idx;
        }
        data_.pop_back();
        reverse_.pop_back();
        lookup_.erase(it);
    }

    const std::vector<Entity>& entities() const { return reverse_; }
    std::vector<T>& raw() { return data_; }
private:
    std::vector<T> data_;
    std::vector<Entity> reverse_;
    std::unordered_map<Entity, std::size_t> lookup_;
};
