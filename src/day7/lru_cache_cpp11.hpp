#pragma once

#include <list>
#include <unordered_map>
#include <utility>
#include <cstddef>

// C++11 风格的 LRUCache：不使用 std::optional / [[nodiscard]] 等 C++17 特性
// 接口区别主要在：get/peek 使用 bool + 输出参数的形式表达“是否命中”。

namespace day7 {

template <class Key, class Value,
          class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>>
class LRUCache11 {
public:
    typedef Key        key_type;
    typedef Value      mapped_type;
    typedef std::size_t size_type;

    explicit LRUCache11(size_type capacity)
        : capacity_(capacity) {}

    // 禁用拷贝，只保留移动
    LRUCache11(const LRUCache11&) = delete;
    LRUCache11& operator=(const LRUCache11&) = delete;

    LRUCache11(LRUCache11&&) noexcept = default;
    LRUCache11& operator=(LRUCache11&&) noexcept = default;

    size_type size() const noexcept { return list_.size(); }
    size_type capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return list_.empty(); }

    void clear() noexcept {
        list_.clear();
        map_.clear();
    }

    // C++11 风格的 get：
    // 命中返回 true，并通过 out 参数返回 value；
    // 未命中返回 false。
    // 注意：get 会更新访问顺序，把命中元素移动到表头。
    bool get(const Key& key, Value& out) {
        typename Map::iterator it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        touch(it);
        out = it->second->second; // 拷贝 Value 返回
        return true;
    }

    // 不改变访问顺序的只读查询（可选接口）
    bool peek(const Key& key, Value& out) const {
        typename Map::const_iterator it = map_.find(key);
        if (it == map_.end()) {
            return false;
        }
        out = it->second->second;
        return true;
    }

    bool contains(const Key& key) const {
        return map_.find(key) != map_.end();
    }

    // 插入或更新 key，对 value 进行拷贝
    void put(const Key& key, const Value& value) {
        typename Map::iterator it = map_.find(key);
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
        typename Map::iterator it = map_.find(key);
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
    typedef std::list<std::pair<Key, Value> >            List;
    typedef typename List::iterator                      ListIt;
    typedef std::unordered_map<Key, ListIt, Hash, KeyEqual> Map;

    size_type capacity_;
    List      list_;
    Map       map_;

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
            std::pair<Key, Value>& back = list_.back();
            map_.erase(back.first);
            list_.pop_back();
        }

        list_.emplace_front(key, std::forward<V>(value)); // 可能抛异常
        map_[key] = list_.begin();                         // 可能抛异常/rehash
    }
};

} // namespace day7
