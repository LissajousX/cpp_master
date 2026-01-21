## Day11：C++17 补充特性

### 理论速览

- 轻量值语义与“也许有值”：`std::optional<T>`
- 代替继承的一种方式：`std::variant<Ts...>` + `std::visit`
- 运行期类型擦除容器：`std::any`
- 零拷贝字符串视图：`std::string_view`
- 现代文件系统操作：`std::filesystem`
- 语法增强：结构化绑定、if/switch init、折叠表达式

目标：

- 会用 `optional`/`variant`/`string_view`/`filesystem` 写出“现代风格”的接口和小工具。
- 理解这些特性出现的动机和常见坑（尤其是 string_view 生命周期）。

---

### std::optional：表示“可能没有值”

#### 1. 动机

- 替代：
  - 返回裸指针（可能为 nullptr）；
  - 返回特殊值（如 -1 / 空字符串）表示失败；
  - 输出参数（通过引用返回额外信息）。

#### 2. 基本用法

```cpp
#include <optional>
#include <string>

std::optional<int> parse_int(const std::string& s) {
    try {
        return std::stoi(s);
    } catch (...) {
        return std::nullopt; // 无值
    }
}

void foo() {
    if (auto v = parse_int("123")) {
        // v 是 std::optional<int>
        int x = *v;      // 解引用
    }
}
```

要点：

- `has_value()` / `operator bool()` 判断是否有值；
- `value_or(default)` 提供默认值；
- 避免滥用：不要把所有返回值都套 optional，只有“无值”是合理状态时才用。

---

### std::variant：类型安全的联合 + 代替继承的小例子

#### 1. 动机

- 代替：
  - `union`（无类型安全、不能放非平凡类型）；
  - 层级很浅、更多是“**几种备选类型中的一种**”而不是“有行为的类层次”。

#### 2. 基本用法

```cpp
#include <variant>
#include <string>
#include <iostream>

using Shape = std::variant<int, double, std::string>; // 只是示例

void print_shape(const Shape& s) {
    std::visit([](const auto& v) {
        std::cout << v << "\n";
    }, s);
}
```

#### 3. 替代继承的小例子（实践题一）

- 传统做法：`struct Circle : Shape { ... }; struct Rect : Shape { ... }; virtual void draw();`
- variant 做法：
  - 定义 `struct Circle { ... }; struct Rect { ... }; using Shape = std::variant<Circle, Rect>;`
  - 用 `std::visit` 实现 `draw`：

```cpp
struct Circle { double r; };
struct Rect   { double w, h; };

using Shape = std::variant<Circle, Rect>;

void draw(const Shape& s) {
    std::visit([](const auto& shape) {
        using T = std::decay_t<decltype(shape)>;
        if constexpr (std::is_same_v<T, Circle>) {
            // 画圆
        } else if constexpr (std::is_same_v<T, Rect>) {
            // 画矩形
        }
    }, s);
}
```

> 实战记忆：**行为不多、类型固定的“和型”（sum type） → variant 更合适；行为丰富、需要扩展子类 → 传统继承更自然。**

---

### std::any：运行期类型擦除容器

- 可以存放任意类型的值，运行期通过 `typeid` / `any_cast` 取出。

```cpp
#include <any>
#include <iostream>

void print_any(const std::any& a) {
    if (a.type() == typeid(int)) {
        std::cout << std::any_cast<int>(a) << "\n";
    } else if (a.type() == typeid(std::string)) {
        std::cout << std::any_cast<std::string>(a) << "\n";
    }
}
```

- 适合：
  - 少量杂项配置、插件接口等；
- 不适合：
  - 性能关键路径（额外分配、type erasure 开销）；
  - 大规模滥用（可读性差）。

---

### std::string_view：零拷贝字符串视图

#### 1. 动机与特性

- 轻量只读视图，不拥有数据：
  - 指针 + 长度；
  - 不负责生命周期。
- 适合：
  - 函数参数接受“任何字符串类/字面量”而不复制；
  - 在一个大字符串上切片做解析。

```cpp
#include <string_view>

void log(std::string_view msg) {
    // 只读，不复制
}

void demo() {
    std::string s = "hello";
    log(s);          // OK
    log("world");   // OK
}
```

#### 2. 生命周期坑（实践题二）

```cpp
std::string_view bad() {
    std::string s = "hello";
    return s; // 返回的 string_view 指向局部已销毁内存，悬垂
}
```

- 规则：
  - 不要返回指向临时/local `std::string` 的 `string_view`；
  - 确保底层字符串活得比 `string_view` 久。

---

### std::filesystem：现代文件系统库

- 头文件：`<filesystem>`，命名空间：`std::filesystem`。

常用：

```cpp
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

int main() {
    fs::path p = ".";
    for (const auto& entry : fs::directory_iterator(p)) {
        std::cout << entry.path() << "\n";
    }
}
```

- 能力：
  - 路径操作：`path` 拼接、父目录、扩展名等；
  - 文件属性：`exists`, `is_regular_file`, `file_size`；
  - 创建/删除：`create_directories`, `remove_all`。

实践题三：写一个小工具，列出给定目录下的所有 `.cpp` 文件及大小。

---

### 其它语法糖：结构化绑定 / if/switch init / 折叠表达式

#### 1. 结构化绑定

```cpp
#include <map>

std::map<int, int> m;

void f() {
    for (auto [k, v] : m) {
        // 直接用 k, v
    }
}
```

- 也常用于从 `std::tuple` / `std::pair` 解包返回值。

#### 2. if / switch init

```cpp
if (auto it = m.find(key); it != m.end()) {
    // it 在 if 作用域内有效
}
```

- 避免把只用于条件判断的变量泄漏到外层作用域。

#### 3. 折叠表达式（可先理解概念）

- 主要用于可变模板参数包的“累积”：

```cpp
template <typename... Args>
auto sum(Args... args) {
    return (args + ...); // ( (arg1 + arg2) + arg3 ) ...
}
```

- 对你设计通用库/工具函数时很有用，但日常业务代码里出现频率相对没那么高。

---

### 小结与练习

- 本日重点：
  - `optional`：合适地表达“有/无值”；
  - `variant`：适合少量备选类型，配合 `visit` 替代部分继承；
  - `any`：作为“逃生舱”少量使用；
  - `string_view`：高性能只读视图，要小心生命周期；
  - `filesystem`：不再自己手撸 `opendir/readdir`；
  - 结构化绑定 / if-init 等提升可读性的小语法。

- 建议练习：
  1. 写一个 `parse_config(std::string_view)`，返回 `std::optional<Config>`，体验 `optional` 带来的接口可读性提升。
  2. 用 `std::variant<Circle, Rect, Triangle>` 重写 Day10 的“图形示例”，用 `std::visit` 实现 `draw`，对比继承版的差异。
  3. 实现一个 `list_dir.cpp`，接受命令行参数目录路径，使用 `std::filesystem` 列出所有文件及大小。
