## Day7：复盘 + 小项目 1（LRU 缓存，强调拷/移与异常安全）

### 理论速览
- LRU（Least Recently Used）缓存：在容量有限的情况下，总是优先淘汰“最久未使用”的元素，常见于本地缓存、数据库连接池、分页置换等场景。
- 典型实现结构：`list + unordered_map`：
  - `list` 维护访问顺序（头部 = 最近使用，尾部 = 最久未使用）。
  - `unordered_map<key, iterator>` 实现 O(1) 查找，`iterator` 指向 `list` 中的节点。
- 复杂度目标：
  - `get(key)`：平均 O(1)，存在则移动节点到表头；不存在返回 miss。
  - `put(key, value)`：平均 O(1)，更新已有 key 或插入新 key，必要时淘汰尾部节点。
- 拷贝/移动语义设计：
  - 明确是否允许拷贝：
    - 小容量、值语义需求明显 → 可以实现拷贝构造/赋值（深拷贝内部 `list` 和 `map`）。
    - 大容量、资源重型对象 → 可以禁用拷贝，仅支持移动（`= delete` 拷贝构造和拷贝赋值，保留移动）。
  - 移动语义：无抛异常地把内部容器的所有权转移给目标对象，源对象置于有效但未指定状态（通常为“空缓存”）。
- 异常安全：
  - 使用 RAII 资源管理（`std::list`/`std::unordered_map` 本身已具备良好异常安全性）。
  - 对外保证至少“基本异常安全”：操作失败不会破坏缓存内部不变式（要么成功完全生效，要么保持老状态，并清晰记录失败原因）。
  - 避免在关键路径上抛异常（比如定制哈希函数/比较器时要谨慎）。

---

### 设计目标与接口草案

#### 1. 功能目标

- 维护一个 `capacity` 容量的 K-V 缓存：
  - `get(key)`：查询 key，命中则返回 value 并更新为“最近使用”；未命中返回“不存在”。
  - `put(key, value)`：插入或更新 key；如容量已满且是新 key，则淘汰一个“最久未使用”的 key。
- 复杂度要求：
  - `get` / `put` 平均 O(1)。
- 语义要求：
  - 支持任意可哈希且可相等比较的 `Key` 类型（默认用 `std::hash<Key>` + `std::equal_to<Key>`）。
  - `Value` 类型要求可移动/可拷贝（根据你的设计选型）。

#### 2. 对外接口（示意）

- 类型模板：
  - `template <class Key, class Value, class Hash = std::hash<Key>, class KeyEqual = std::equal_to<Key>> class LRUCache;`
- 核心成员函数（草案）：
  - 构造：
    - `explicit LRUCache(size_t capacity);`
    - 可选：带自定义哈希/比较器的构造。
  - 基本操作：
    - `bool get(const Key& key, Value& out);`  
      返回是否命中；命中则写入 `out` 并将该 key 移动到“最近使用”。
    - `std::optional<Value> get(const Key& key) const;`（可选重载）
    - `void put(const Key& key, const Value& value);`  
      复制 value（可能抛异常）。
    - `void put(const Key& key, Value&& value);`  
      移动 value（减少拷贝）。
  - 容量/状态：
    - `size_t size() const noexcept;`
    - `size_t capacity() const noexcept;`
    - `bool empty() const noexcept;`
    - `void clear() noexcept;`  
      清空缓存，不抛异常。

> 注：实际实现时可以根据精力删减接口，比如只保留一个 `get` 版本，`put` 也可以只支持按值传入。

---

### 内部数据结构与不变式

#### 1. 内部成员建议

- 类型别名：
  - `using List = std::list<std::pair<Key, Value>>;`
  - `using ListIt = typename List::iterator;`
  - `using Map = std::unordered_map<Key, ListIt, Hash, KeyEqual>;`
- 成员变量：
  - `size_t capacity_;`  
    最大容量（元素个数）。
  - `List list_;`  
    存放实际 K-V 对，表头 = 最近使用，表尾 = 最久未使用。
  - `Map map_;`  
    key → list 节点迭代器，供 O(1) 查找与定位。

#### 2. 关键不变式（面试可直接说）

- `list_` 中的元素顺序代表访问时间顺序：
  - 表头 = 最近一次被访问/插入的元素；
  - 表尾 = 最久未访问的元素（LRU 淘汰对象）。
- `map_` 中的每个 entry 满足：
  - `map_[key]` 是一个指向 `list_` 某节点的有效迭代器；
  - 该节点的 `first` == `key`。
- `size_ == list_.size() == map_.size()`（如果你显式维护 `size_` 的话）。
- `size() <= capacity_`。

只要任何操作结束时这些不变式都成立，你的 LRU 就是“健康”的。

---

### 操作流程设计

#### 1. get(key)

伪代码（逻辑层面）：

1. 在 `map_` 中查找 key：
   - 如果未命中：返回 false / 空 `optional`。
   - 如果命中：拿到迭代器 it。
2. 从 `list_` 中把该节点移动到表头（`list_.splice(list_.begin(), list_, it);`）。
3. 输出或返回 `it->second`。

复杂度：平均 O(1)（`unordered_map` 查找 + `list` 的 splice O(1)）。

#### 2. put(key, value)

分两种情况：

1. key 已存在（更新）：
   - 通过 `map_` 查到 it；
   - 更新 `it->second = value`（或移动赋值）；
   - 把该节点移动到 `list_.begin()`（最近使用）。
2. key 不存在（插入）：
   - 如果 `size() == capacity_`：
     - 找到 `list_.back()`（尾部元素）；
     - 从 `map_` 中 `erase` 掉对应 key；
     - 从 `list_` 中 `pop_back()` 删除该节点。
   - 在 `list_.front` 处 `emplace_front(key, value)`；
   - 在 `map_` 中记录 `key -> list_.begin()`。

关键点：保证无论如何结束时，不变式仍然成立。

---

### 拷贝 / 移动 / 异常安全设计

#### 1. 拷贝语义选项

- **选项 A：禁用拷贝，仅支持移动**（简单、现实项目常用）：
  - `LRUCache(const LRUCache&) = delete;`
  - `LRUCache& operator=(const LRUCache&) = delete;`
  - 提供：
    - `LRUCache(LRUCache&&) noexcept = default;`
    - `LRUCache& operator=(LRUCache&&) noexcept = default;`
  - 面试可以说：缓存往往不需要值拷贝，避免无意义的大量元素拷贝；
    重建一个新的缓存更简单。

- **选项 B：支持拷贝语义**：
  - 拷贝构造：逐元素拷贝 `list_` 和 `map_`，保持顺序一致。
  - 拷贝赋值：可以用 copy-and-swap 提供强异常安全：
    - 先拷贝一份临时对象；
    - 再与当前对象交换内部资源（`swap` 不抛异常）。

可以在 Day7 实现时二选一（建议你先实现 A，理解移动和资源控制，再尝试 B 作为进阶练习）。

#### 2. 异常安全要点

- 插入/更新时可能抛异常的地方：
  - `Value` 的拷贝/移动构造、赋值；
  - `unordered_map` 的 rehash（分配失败、构造失败）。
- 一般策略：
  - 先准备好新节点/新 value（如果失败，让异常抛出，旧状态不变）；
  - 再更新 `map_` 和 `list_`，保持更新步骤尽量“要么全部成功，要么不改变原值”。

> 对于面试，只要你能清晰说出“哪些地方可能抛异常 + 我怎么保证不变式不被破坏”，就已经是加分项。

---

### 最小代码示例（接口与骨架）

```cpp
#include <list>
#include <unordered_map>
#include <optional>
#include <utility>

template <class Key, class Value,
          class Hash = std::hash<Key>,
          class KeyEqual = std::equal_to<Key>>
class LRUCache {
public:
    explicit LRUCache(std::size_t capacity)
        : capacity_(capacity) {}

    // 禁用拷贝，只保留移动（示例选项 A）
    LRUCache(const LRUCache&) = delete;
    LRUCache& operator=(const LRUCache&) = delete;

    LRUCache(LRUCache&&) noexcept = default;
    LRUCache& operator=(LRUCache&&) noexcept = default;

    std::size_t size() const noexcept { return list_.size(); }
    std::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return list_.empty(); }

    void clear() noexcept {
        list_.clear();
        map_.clear();
    }

    std::optional<Value> get(const Key& key) {
        auto it = map_.find(key);
        if (it == map_.end()) {
            return std::nullopt;
        }
        // 把节点移动到表头（最近使用）
        list_.splice(list_.begin(), list_, it->second);
        return it->second->second; // 拷贝一份 Value 返回
    }

    void put(const Key& key, const Value& value) {
        auto it = map_.find(key);
        if (it != map_.end()) {
            // 更新已有 key
            it->second->second = value;     // 可能抛异常
            list_.splice(list_.begin(), list_, it->second);
            return;
        }

        // 插入新 key，必要时淘汰尾部
        if (list_.size() == capacity_) {
            // 淘汰最久未使用的元素（尾部）
            auto& back = list_.back();
            map_.erase(back.first);
            list_.pop_back();
        }

        list_.emplace_front(key, value);    // 可能抛异常
        map_[key] = list_.begin();          // 可能抛异常/rehash
    }

private:
    using List      = std::list<std::pair<Key, Value>>;
    using ListIt    = typename List::iterator;
    using Map       = std::unordered_map<Key, ListIt, Hash, KeyEqual>;

    std::size_t capacity_;
    List list_;
    Map  map_;
};
```

> 说明：
> - 上面只是一个简化的骨架示例，真实实现时需要考虑：
>   - `capacity_ == 0` 时的行为；
>   - 是否提供 `put(Key&&, Value&&)` 等移动优化接口；
>   - 是否对 `get` 提供 `const` 版本（可能需要去掉“更新顺序”的副作用或使用 `mutable`）。
> - 你可以在 `src/day7/lru_cache.hpp/.cpp` 里整理出一个更完整的版本。

---

### 练习题

1) 按本文档中的设计，实现一个模板类 `LRUCache<Key, Value>`，并在 `main` 中编写简单的单元测试：
   - 重复访问同一个 key，检查顺序是否保持正确；
   - 插入超过容量的元素，检查最久未访问元素是否被淘汰。
2) 在当前实现基础上，增加：
   - `bool contains(const Key& key) const;`
   - `std::size_t hit_count() const;` / `miss_count() const;`（可选）。
   思考：这些统计信息会如何影响线程安全？
3) 修改 `put` 接口，增加一个 `put_if_absent` 版本：
   - 只有当 key 不存在时插入；
   - 如 key 已存在，返回 false 且不改变其 value 和顺序。
4) 为 LRU 缓存实现一个简单的“钩子”机制：
   - 在元素被淘汰时，调用用户提供的回调（例如记录日志或释放外部资源）。
   - 思考：回调抛异常时如何处理？
5) 用 `std::map` 代替 `std::unordered_map` 实现一个“有序 LRU”（理论上没必要，但可以作为练习），比较：
   - 查找/插入复杂度（log n vs O(1)）；
   - 实际性能和代码复杂度。

---

### 自测要点（回答并尽量用自己的话复述）

- LRU 缓存的语义是什么？为什么需要“最近使用”的概念？举一个你熟悉的业务场景。
- 为什么常见实现选择 `list + unordered_map` ？分别解决了什么问题？
- `get`/`put` 各自的时间复杂度是多少？在什么情况下可能退化？
- 在你的设计里，LRUCache 的拷贝/移动策略是怎样的？为什么这样设计？
- 你的实现提供了怎样的异常安全保证？当 `put` 过程中发生异常时，缓存内部状态是否仍然满足不变式？
- 如果以后需要让 LRU 缓存支持多线程并发访问，你会优先考虑哪种方案（全局锁、分段锁、读写锁、无锁结构）？为什么？
