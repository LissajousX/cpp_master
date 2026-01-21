## Day13：异常边界与 API 设计 + OOP 设计取舍

### 理论速览

- 异常 vs `std::error_code`：
  - 异常：用于“非预期错误”，跨层传播方便，但要注意异常安全与边界；
  - `error_code`：显式错误返回，更适合底层库/跨 ABI 接口。
- `noexcept` 约定：
  - 析构函数几乎总是 `noexcept`；
  - 移动操作若 `noexcept`，容器在优化上会更激进（如 `vector` 优先移动而非拷贝）。
- Pimpl（Pointer to implementation）：
  - 隐藏实现细节，减少头文件依赖，改善编译时间和 ABI 稳定性。
- 接口最小可见性：
  - 限制 `public`/`protected`，优先 `private`；
  - 用内部命名空间、匿名命名空间隐藏仅在本翻译单元使用的设施。
- OOP 设计取舍：
  - 组合 vs 继承；
  - 虚函数开销/可维护性；
  - 接口稳定性与二进制兼容性。

目标：

- 能为同一个组件设计“异常版 API”与 `error_code` 版 API；
- 理解何时适合用 pimpl、何时直接暴露具体类型即可；
- 在 LRU/队列这类数据结构上实践“接口最小可见性”。

---

### 异常 vs std::error_code

#### 1. 异常的优点与适用场景

- 适合：
  - 逻辑上“罕见”的错误（例如磁盘故障、严重参数错误）；
  - 需要跨多层传播错误信息、在高层统一处理的场景。
- 优点：
  - 正常路径代码更简洁，不需要层层传递错误码；
  - 可以携带丰富上下文（自定义异常类型）。

#### 2. error_code 的优点与适用场景

- 适合：
  - 底层库、系统 API 适配层；
  - 不希望抛异常的环境（如某些实时系统或对异常支持较差的嵌入式环境）；
  - 边界清晰的 API（例如返回 `bool` + `std::error_code&`）。
- 优点：
  - 错误处理语义更显式；
  - 不依赖异常机制，行为更可预测。

#### 3. 示例：同一 API 的异常版与 error_code 版

以简单文件读取为例：

```cpp
#include <string>
#include <system_error>

std::string read_file_throw(const std::string& path) {
    // 失败时抛异常
    // ...
    throw std::runtime_error("read failed");
}

bool read_file_ec(const std::string& path, std::string& out, std::error_code& ec) {
    // 失败时设置 ec，返回 false
    // ...
    ec = std::make_error_code(std::errc::no_such_file_or_directory);
    return false;
}
```

> 指南：
> - 库内部可以统一用异常，在边界层适配为 `error_code` 接口；
> - 或反之：内部用 `error_code`，在更上层封一层异常版 API。

---

### noexcept 与移动语义

- 析构函数默认应为 `noexcept`（不抛异常），否则在栈展开过程中再抛异常会导致 `std::terminate`。
- 移动构造/赋值若被标记为 `noexcept`，标准容器可以在重分配时选择“移动而不是拷贝”，性能更好：

```cpp
struct Buffer {
    Buffer(Buffer&&) noexcept;            // 更利于容器优化
    Buffer& operator=(Buffer&&) noexcept; 
};
```

- 一般经验：
  - 如果移动操作本身不会抛异常（大部分资源句柄/智能指针类型），就标记为 `noexcept`。

---

### Pimpl 模式与接口稳定性

#### 1. Pimpl 的基本形式

```cpp
// header
class Widget {
public:
    Widget();
    ~Widget();

    void do_something();

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
```

```cpp
// source
struct Widget::Impl {
    int x;
    // 更多私有实现细节
};

Widget::Widget() : impl_(std::make_unique<Impl>()) {}
Widget::~Widget() = default;

void Widget::do_something() {
    impl_->x = 42;
}
```

- 优点：
  - 头文件不暴露成员，实现改动不引起 ABI 变化（对动态库重要）；
  - 降低编译依赖，修改实现不需要重新编译所有使用者。
- 缺点：
  - 需要一次额外间接访问（指针），有轻微开销；
  - 代码量略多，调试不如直观。

> 何时用：
> - 面向外部使用的稳定库/SDK；
> - 接口需要保持二进制兼容性（SO/DLL），实现又可能频繁变化。

---

### 接口最小可见性与 OOP 设计取舍

- 原则：
  - 尽量缩小 `public` 接口，只暴露稳定必要的最小集合；
  - 内部实现细节藏在 `private`，必要时用 `friend` / 内部 helper。
- 组合 vs 继承：
  - 先考虑组合（has-a），再考虑继承（is-a）；
  - 继承容易形成紧耦合，接口一改动影响整棵树。
- 虚函数与层次：
  - 虚函数提供扩展点，但要控制层次深度；
  - 可以用 `final` 杜绝无意义的进一步继承。

#### 简单示例：为 LRU 封一层最小 API

- 内部实现可能是 `list + unordered_map`；
- 对外只暴露：构造、`get`、`put`、容量查询等，不暴露内部容器类型。

```cpp
class LruCache {
public:
    explicit LruCache(std::size_t capacity);

    bool get(const Key& key, Value& out) const;       // error_code 版可以返回 bool + ec
    void put(const Key& key, const Value& value);

    std::size_t size() const noexcept;

private:
    // 内部实现细节隐藏
};
```

---

### 实践建议

1. **为 LRU 或队列设计两套 API：异常版与 error_code 版**：
   - 异常版：`Value get(const Key& key)`，未命中时抛异常；
   - error_code 版：`bool get(const Key& key, Value& out, std::error_code& ec)`。
   - 比较调用方代码可读性，以及错误处理路径的差异。

2. **为某个中等复杂度类应用 pimpl**：
   - 先写一个直接暴露成员的类；
   - 再改为 pimpl，观察头文件/实现文件的变化；
   - 思考如果作为动态库导出，两种写法在 ABI 稳定性上的区别。

3. **写一小段演示“接口最小可见性”的代码**：
   - 故意写一个暴露很多 public 成员的类；
   - 再重构成只暴露少量方法，内部实现放到私有结构或 helper 函数中。

---

### 小结

- 异常与 `error_code` 各有适用场景，关键是 **在模块边界处统一风格**；
- 正确使用 `noexcept` 能帮助标准容器做更好的优化，特别是移动操作；
- Pimpl 是为“接口稳定性与编译隔离”服务的，不是为“炫技”服务的；
- 接口设计应当从“调用方体验”和“长期演进成本”出发，
  控制 public 面的大小，避免过度暴露实现细节。
