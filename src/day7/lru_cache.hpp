#pragma once

#include <list>
#include <unordered_map>
#include <optional>
#include <utility>
#include <cstddef>

// LRUCache: 最近最少使用缓存
// 典型用法：
//   LRUCache<int, std::string> cache(3);
//   cache.put(1, "one");
//   auto v = cache.get(1); // 命中
//
// 设计要点：
// - list 维护访问顺序（头 = 最近使用，尾 = 最久未使用）
// - unordered_map 实现 O(1) 查找，value 是指向 list 节点的迭代器
// - 禁用拷贝，仅允许移动（避免大规模无意义拷贝）

namespace day7 {

template <class Key, class Value,
          class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>>
class LRUCache {
public:
    using key_type        = Key;
    using mapped_type     = Value;
    using size_type       = std::size_t;

    explicit LRUCache(size_type capacity)
        : capacity_(capacity) {}

    // 禁用拷贝，只保留移动
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    LRUCache(LRUCache&&) noexcept = default;
    LRUCache& operator=(LRUCache&&) noexcept = default;

    [[nodiscard]] size_type size() const noexcept { return list_.size(); }
    [[nodiscard]] size_type capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return list_.empty(); }

    void clear() noexcept {
        list_.clear();
        map_.clear();
    }

    // 命中返回 value，未命中返回 std::nullopt
    // 注意：get 会更新访问顺序，把命中元素移动到表头
    std::optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        touch(it);
        return it->second->second; // 拷贝 Value 返回
    }

    // 不改变访问顺序的只读查询（可选接口）
    std::optional<Value> peek(const Key& key) const {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        return it->second->second;
    }

    bool contains(const Key& key) const {
        return map_.find(key) != map_.end();
    }

    // 插入或更新 key，对 value 进行拷贝
    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            // 更新已有 key
            it->second->second = value; // 可能抛异常
            touch(it);
            return;
        }

        insert_new(key, value);
    }

    // 插入或更新 key，优先使用移动
    void put(const Key& key, Value&& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            it->second->second = std::move(value); // 可能抛异常
            touch(it);
            return;
        }

        insert_new(key, std::move(value));
    }

    // 仅当 key 不存在时插入，返回是否插入成功
    bool put_if_absent(const Key& key, const Value& value) {
        if (contains(key)) {
            return false;
        }
        insert_new(key, value);
        return true;
    }

private:
    using List   = std::list<std::pair<Key, Value>>;
    using ListIt = typename List::iterator;
    using Map    = std::unordered_map<Key, ListIt, Hash, KeyEqual>;

    size_type capacity_;
    List list_;
    Map  map_;

    // 将命中的元素移动到表头（最近使用）
    void touch(typename Map::iterator it) {
        list_.splice(list_.begin(), list_, it->second);
    }

    // 插入一个全新的 key（调用前保证 key 不在 map_ 中）
    template <class V>
    void insert_new(const Key& key, V&& value) {
        if (capacity_ == 0) {
            return; // 容量为 0，则不缓存任何内容
        }

        if (list_.size() == capacity_) {
            // 淘汰最久未使用的元素（尾部）
            auto& back = list_.back();
            map_.erase(back.first);
            list_.pop_back();
        }

        list_.emplace_front(key, std::forward<V>(value)); // 可能抛异常
        map_[key] = list_.begin();                         // 可能抛异常/rehash
    }
};

} // namespace day7
