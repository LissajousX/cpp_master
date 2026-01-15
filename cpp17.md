## 0. 大框架：从 C++11 到 C++17

你可以在面试里这样开头：

> C++11 带来了现代 C++ 的“大爆炸”：auto、range-for、lambda、move 语义、智能指针、`std::thread` 等。  
> C++17 主要是在这一套基础上进一步补齐语法和库设施，比如结构化绑定、`if constexpr`、`std::optional`、`std::variant`、`std::string_view`、`std::filesystem`，以及一些语言层面的改进（fold expression、CTAD 等）。

然后挑你熟的几个展开说。

---

## 1. 结构化绑定（structured binding）

- **作用**：把一个 `pair/tuple/struct` 拆成多个变量，代码更清晰。
- 关键语法：`auto [x, y] = some_pair;`

示例：

```cpp
std::map<int, std::string> m{{1, "one"}, {2, "two"}};

for (const auto& [key, value] : m) {  // C++17
    std::cout << key << " -> " << value << "\n";
}
```

对比 C++11：

```cpp
for (const auto& kv : m) {
    std::cout << kv.first << " -> " << kv.second << "\n";
}
```

面试可说：**读 map 时不用再写 `first/second`，直接 `[key, value]` 更直观。**

---

## 2. `if constexpr` —— 编译期条件（constexpr if）

- **作用**：在模板里根据编译期条件选择不同分支，**未选中的分支不会参与编译**。
- 解决 C++11 里常见的 “SFINAE + enable_if 代码很丑” 问题。

例子：打印容器，若有 [size()](cci:1://file:///home/lisa/cpp_master/src/day7/lru_cache.hpp:40:4-40:74) 就打印 size，否则不打印：

```cpp
template <class T>
void print_info(const T& x) {
    if constexpr (requires (const T& t) { t.size(); }) {  // 伪代码式概念写法
        std::cout << "size = " << x.size() << "\n";
    } else {
        std::cout << "no size()\n";
    }
}
```

真实代码一般用 trait/SFINAE 写，但核心是：

> `if constexpr (条件)`：编译器只会编译为 true 的那一支，另一支即使有语法错误也没关系。

面试关键词：**“编译期 if，简化模板元编程逻辑”**。

---

## 3. 折叠表达式（fold expressions）—— 折叠参数包

- **用途**：对可变参数模板的参数包做“累加/累与/打印”等操作，写法更简洁。
- 核心语法：`(args + ...)`、`(... && args)` 等。

示例：把多个参数依次输出：

```cpp
template <class... Args>
void print_all(const Args&... args) {
    (std::cout << ... << args) << '\n';   // 从左到右折叠输出
}
```

对比 C++11 里可能要写递归模板或 initializer list 技巧。

---

## 4. 类模板实参推导（CTAD：Class Template Argument Deduction）

- **缩写解释**：CTAD = Class Template Argument Deduction（类模板实参推导）。
- **作用**：像函数模板那样，对类模板也能推导模板参数，不用手写 `<int>` 之类。

示例：

```cpp
std::pair p(1, 2.0);        // C++17：推导为 std::pair<int, double>
std::tuple t(1, 2.0, "hi"); // 推导出对应的 tuple 类型
```

自定义类型也可以配合推导指引（你 day4 已经写过）：

```cpp
template<class T>
struct Box {
    T value;
    Box(T v) : value(std::move(v)) {}
};

Box b(1);        // C++17: 推导为 Box<int>
```

面试可以说：**“CTAD 让一些模板类型使用起来更自然，但要注意可读性”**。

---

## 5. inline 变量（inline variables）

- **作用**：解决“头文件里定义一个全局变量或静态成员，会导致 ODR（One Definition Rule）违反”的问题。
- C++11 里：只能在头文件里声明，在某个 .cpp 里定义一次；或用 `static` 每个 TU 一份。
- C++17：`inline` 变量允许在头文件里 **直接定义**，多个翻译单元共享同一个实体。

```cpp
// header.hpp
struct Config {
    int v = 0;
};

inline Config global_config;   // C++17：可以在头文件中定义
```

这个点在你写 library/全局单例时会遇到。

---

## 6. `std::optional` / `std::variant` / `std::any`

这三个属于 **“值语义的可选值 / 和类型安全 union / 类型擦除”** 三兄弟，非常面试常见。

### 6.1 `std::optional<T>`

- 语义：**“要么有一个 T，要么啥都没有”**。
- 用于表达“返回值可能不存在”，替代“返回 nullptr / 特殊值”。

你已经在 LRU 里见过：

```cpp
std::optional<Value> get(const Key& key);
```

---

### 6.2 `std::variant<Ts...>` —— 类型安全的联合（替代 `union`）

- 语义：在多个候选类型中，**“当前是其中之一”**。
- 例如：一个 AST 节点可能是 `int`、`double` 或 `std::string`：

```cpp
std::variant<int, double, std::string> v;

v = 42;
v = 3.14;
v = std::string("hi");

// 访问：用 std::visit + lambda
std::visit([](auto&& x) {
    std::cout << x << "\n";
}, v);
```

好处：**比传统 `union` 安全、有类型信息；比基类指针简单，不需要虚函数。**

---

### 6.3 `std::any`

- 语义：**“任何类型的值”**，运行时才知道真实类型，有点像 type-erased box。
- 用于做“弱类型”的容器/事件总线之类，一般面试了解即可。

---

## 7. `std::string_view` —— 轻量字符串视图

- **作用**：不拷贝字符串，只是看一段已有字符的“视图”（指针+长度），支持切片。
- 常用于：解析、日志、函数参数接收“读字符串”而不复制。

示例：

```cpp
void print_prefix(std::string_view sv) {
    if (sv.size() > 3) {
        std::cout << sv.substr(0, 3) << "\n";
    }
}

std::string s = "hello";
print_prefix(s);         // 不拷贝
print_prefix("world");   // 字面量也可以
```

要点：

- **不拥有数据**，只是“借用”；  
- 要注意不要返回指向临时字符串的 `string_view`（悬空引用）。

---

## 8. `std::filesystem` —— 文件系统库

- C++17 把 `std::experimental::filesystem` 正式纳入标准。
- 提供：
  - `std::filesystem::path`：路径类型；
  - `exists` / `is_directory` 等检查 API；
  - 目录遍历：`directory_iterator`；
  - 文件大小、创建/删除等操作。

示例：

```cpp
#include <filesystem>
namespace fs = std::filesystem;

void list_files(const fs::path& dir) {
    for (auto& entry : fs::directory_iterator(dir)) {
        std::cout << entry.path() << "\n";
    }
}
```

面试可以说：**“C++17 之后，不再需要依赖 boost.filesystem 来做跨平台文件操作”**。

---

## 9. 并行算法（Parallel Algorithms）

- 在 `<algorithm>` 里的很多算法（`sort`, `for_each`, `reduce` 等）多了一个重载：
  - 第一个参数是执行策略（execution policy）：
    - `std::execution::seq`：串行（默认）
    - `std::execution::par`：多线程并行
    - `std::execution::par_unseq`：并行 + 向量化
- 示例（概念上）：

```cpp
#include <execution>
#include <vector>
#include <algorithm>

std::vector<int> v = /* ... */;
std::sort(std::execution::par, v.begin(), v.end());
```

实际工程里依赖实现支持（libstdc++/libc++ + 编译器），但面试知道这个趋势即可。

---

## 10. 面试时可以怎么总结 C++17 相比 C++11？

你可以准备一段“总括 + 挑重点”的说法，例如：

> 在 C++11 打下现代 C++ 基础之后，C++17 主要做了两件事：  
> 一是**简化模板和语言语法**，比如结构化绑定、`if constexpr`、fold expression、CTAD、inline 变量；  
> 二是**补齐标准库组件**，比如 `std::optional` / `std::variant` / `std::any`、`std::string_view`、`std::filesystem` 和并行算法等。  
> 我常用的几个是：结构化绑定、`if constexpr`、`optional`、`string_view` 和 filesystem，可以展开说具体场景。

---