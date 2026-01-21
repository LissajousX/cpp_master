## Day12：性能与调优

### 理论速览

- 小对象优化与对象布局：
  - 尽量保持类型简单、紧凑，减少不必要的虚函数/指针追踪。
  - 重视对象大小和对齐，避免无意义的 padding。
- 容器与内存管理：
  - 预留容量（`reserve`）、选择合适容器（`vector` vs `list` 等）。
  - 避免不必要的拷贝/临时对象，合理使用 `emplace_*` 和移动语义。
- cache 局部性（locality）：
  - 顺序访问连续内存优于随机访问链表/树；
  - 数据与代码布局会影响性能。
- 基准测试（micro benchmark）思路：
  - 使用 `std::chrono`；预热、重复多次、控制变量；
  - 注意编译器优化（防止把待测代码删掉）。

目标：

- 能写出简单的 micro benchmark，对比 reserve vs 不 reserve；
- 对容器选择和 cache 局部性有定性认识，不再盲目使用 list/map。

---

### 小对象优化与对象布局

- 对象越小、越简单，CPU cache 命中率越高：
  - 避免在热路径对象里塞入很少用的大字段（可以拆到旁路结构）；
  - 合理使用结构体重排（手工调整成员顺序，减少 padding）。

```cpp
struct Bad {
    char  c;
    double d;
    int   i;
};

struct Better {
    double d;
    int    i;
    char   c;
};
```

- 一般不需要疯狂手调布局，但要有“对象尺寸与访问模式是有成本的”这个意识。

---

### 容器：reserve 与避免不必要拷贝

#### 1. reserve 减少重分配

```cpp
std::vector<int> v;
for (int i = 0; i < N; ++i) {
    v.push_back(i); // 多次扩容、拷贝/移动
}

std::vector<int> v2;
v2.reserve(N);
for (int i = 0; i < N; ++i) {
    v2.push_back(i); // 基本只分配一次
}
```

- `reserve` 不改变 size，只预留 capacity，减少 `realloc + 元素搬迁` 次数；
- 对元素类型拷贝/移动成本高时（如 string/自定义大对象），收益更明显。

#### 2. 避免不必要拷贝/临时

- 参数传递：
  - 大对象只读参数 → `const T&`；
  - 需要转移所有权 → `T&&` + `std::move`；
- 返回值：
  - 现代 C++ 有 RVO + 移动，一般直接按值返回即可；
- `emplace_back` 直接在容器内部构造：

```cpp
std::vector<std::string> v;
v.emplace_back("hello");                 // 直接构造在 vector 里
v.push_back(std::string("world"));     // 临时对象再移动/拷贝
```

> 指南：优先写清晰代码，在发现热点后再做针对性优化。

---

### cache 局部性与容器选择

- CPU 读取内存时按 cache line（64 字节左右）为单位：
  - **顺序访问连续内存** → 预取效果好，少 cache miss；
  - **指针链式结构** → 每次跳转可能导致 miss。

#### vector vs list 的典型对比

- `std::vector`：
  - 连续内存，顺序遍历很快；
  - 中间插入/删除 O(N)。
- `std::list`：
  - 每个节点单独分配，插入/删除局部 O(1)；
  - 遍历性能常常比 vector 差很多。

> 经验法则：
> - 绝大多数场景，先选 `vector`；
> - 只有当“频繁在中间插入/删除 + 元素很大”时，才认真评估 `list`/`deque` 等。

---

### 用 chrono 写一个简易 micro benchmark

#### 1. 通用 bench 函数

```cpp
#include <chrono>
#include <functional>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

void bench(const std::string& name, const std::function<void()>& fn, int iters) {
    // 预热
    fn();

    auto start = Clock::now();
    for (int i = 0; i < iters; ++i) {
        fn();
    }
    auto end = Clock::now();

    std::chrono::duration<double, std::micro> us = end - start;
    std::cout << name << ": " << (us.count() / iters) << " us per iter\n";
}
```

- 注意：
  - `fn` 内部要有副作用或把结果写到 volatile/全局变量，避免被优化掉；
  - 反复运行、对比不同编译选项（O0/O2），感受优化影响。

---

### 实践：reserve vs 不 reserve 基准

#### 示例代码

```cpp
#include <vector>
#include <chrono>
#include <iostream>

using Clock = std::chrono::high_resolution_clock;

void bench_push_no_reserve(std::size_t N) {
    std::vector<int> v;

    auto start = Clock::now();
    for (std::size_t i = 0; i < N; ++i)
        v.push_back(static_cast<int>(i));
    auto end = Clock::now();

    std::chrono::duration<double, std::milli> ms = end - start;
    std::cout << "no reserve: " << ms.count() << " ms\n";
}

void bench_push_reserve(std::size_t N) {
    std::vector<int> v;
    v.reserve(N);

    auto start = Clock::now();
    for (std::size_t i = 0; i < N; ++i)
        v.push_back(static_cast<int>(i));
    auto end = Clock::now();

    std::chrono::duration<double, std::milli> ms = end - start;
    std::cout << "with reserve: " << ms.count() << " ms\n";
}

int main() {
    std::size_t N = 10'000'000;
    bench_push_no_reserve(N);
    bench_push_reserve(N);
}
```

- 运行多次，观察两个版本耗时对比；
- 可以把 `int` 换成 `std::string`，体会对“大对象”影响更大。

---

### 小结与练习

- 本日重点：
  - 对象/容器大小与布局会直接影响 cache 行为；
  - `reserve`、`shrink_to_fit`、合适的容器选择能带来“白捡的性能”；
  - micro benchmark 要严谨：预热、重复、避免被优化掉。

- 建议练习：
  1. 修改 benchmark，对比 `std::vector<int>` vs `std::list<int>` 的顺序遍历时间；
  2. 写一个简单的 `struct`，尝试不同成员顺序，观察 `sizeof` 和对齐的变化；
  3. 为你自己的某个小工具（如 Day8 队列、Day7 LRU）写一个简单基准，测量不同参数下的性能，写下观察结论。
