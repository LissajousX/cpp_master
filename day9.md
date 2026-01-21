## Day9：原子与内存序

### 理论速览

- `std::atomic`：
  - 提供**单个变量**在多线程环境下的原子读写/修改操作，避免数据竞争（data race）。
  - 适用场景：计数器、标志位、简单状态机、指针交换等。
  - 不能替代 `mutex` 管理复杂对象的一致性（多个字段、不变量仍需锁或更复杂协议）。

- C++ 内存模型中的 `memory_order`：
  - 控制的是**原子操作前后的普通读写在不同线程之间的可见性和重排序约束**。
  - 常用枚举值：
    - `memory_order_relaxed`：只保证原子性，不保证跨线程顺序/可见性。
    - `memory_order_acquire`：通常用于 `load`，保证**后面的读写不会被重排到它前面**。
    - `memory_order_release`：通常用于 `store`，保证**前面的读写不会被重排到它后面**。
    - `memory_order_acq_rel`：既有 acquire 又有 release 语义，多用于读写合一操作（如 `fetch_add`、CAS）。
    - `memory_order_seq_cst`：顺序一致性，默认值，语义最强、推理最简单。

- 释放序 / 可见性（happens-before）：
  - 典型模式：
    - 线程 A：写数据 → `flag.store(true, std::memory_order_release);`
    - 线程 B：`while (!flag.load(std::memory_order_acquire)) {}` → 然后读取数据
  - 一旦 B 看到 `flag == true`，就**一定能看到 A 在 `store(release)` 之前写入的数据**。

---

### std::atomic 基础

#### 1. 基本使用与类型

- 标量类型（`bool`、整数、指针等）通常都可以直接做 `std::atomic<T>`：

```cpp
#include <atomic>

std::atomic<int> counter{0};

void inc() {
    ++counter;                  // 等价于 counter.fetch_add(1)
}

bool flag = false;             // 非原子（示例对比用）
std::atomic<bool> done{false}; // 原子标志位
```

- 常见成员函数：
  - `load(memory_order)` / `store(value, memory_order)`
  - `fetch_add` / `fetch_sub` / `fetch_or` / `fetch_and` / `exchange`
  - `compare_exchange_weak` / `compare_exchange_strong`

> 实战建议：
> - 入门时不写 `memory_order` 参数，使用默认的 `seq_cst`，确保语义最强、最安全；
> - 熟悉后再在“高频计数器”等局部优化到 `relaxed` 或 `acquire/release`。

---

### 三种典型内存序模式

#### 1. `memory_order_relaxed`：仅原子性，不关心顺序

- 适合**统计/计数**场景：只关心最终结果，不依赖精确先后顺序。

```cpp
#include <atomic>
#include <thread>

std::atomic<long long> counter{0};

void worker() {
    for (int i = 0; i < 1000000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

- 保证：
  - 不会发生 data race，最终 `counter` 等于各线程加和。
- 不保证：
  - 谁先谁后、是否可以推断“看到某个计数值就意味着其它变量也已经更新”。

#### 2. `release` + `acquire`：发布数据 + 就绪标志

- 典型“发布-订阅”模式：一个线程写好数据再设标志，另一个线程等标志变真再读数据。

```cpp
#include <atomic>

struct Data { int x; int y; };

Data g_data;
std::atomic<bool> ready{false};

void producer() {
    g_data.x = 42;
    g_data.y = 100;
    ready.store(true, std::memory_order_release); // 发布“数据就绪”
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) {
        // 忙等或适当 sleep/yield
    }
    int a = g_data.x;  // 一定看到 42
    int b = g_data.y;  // 一定看到 100
}
```

- `store(release)` + `load(acquire)` 建立 happens-before：
  - producer 中，写 `g_data` 发生在 `store(release)` 之前；
  - consumer 中，读 `g_data` 发生在 `load(acquire)` 之后；
  - 看到 `ready == true` 就意味着能看到完整的数据写入。

#### 3. `memory_order_seq_cst`：默认 + 最简单

- 所有 `std::atomic` 操作默认使用 `seq_cst`：

```cpp
std::atomic<int> x{0};

void f() {
    x.store(1);      // 等价于 x.store(1, std::memory_order_seq_cst)
    int v = x.load();
}
```

- 语义：所有 `seq_cst` 原子操作在所有线程看来像是按某个全局总顺序执行，方便人类推理。
- 建议：
  - **不确定选啥时就用默认 `seq_cst`**；先保证正确，再考虑性能优化。

---

### 实践一：无锁计数器（基于 std::atomic）

目标：写一个多线程计数器示例，对比：

- 使用 `std::atomic`（无锁计数器）。
- 如果用普通 `int` 不加锁，会被 TSan 报 data race。

#### 代码示例

```cpp
#include <atomic>
#include <thread>
#include <vector>
#include <iostream>

constexpr int kThreads = 4;
constexpr int kIters   = 1000000;

std::atomic<long long> counter{0};

void worker() {
    for (int i = 0; i < kIters; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}

int main() {
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i)
        threads.emplace_back(worker);

    for (auto &t : threads)
        t.join();

    std::cout << "counter = " << counter.load() << "\n";
}
```

练习步骤：

1. 先用 `std::atomic<long long>` + `memory_order_relaxed` 跑一遍，确认结果是 `kThreads * kIters`。
2. 把 `counter` 改成普通 `long long`，去掉 `atomic`，用 ThreadSanitizer（TSan）运行：
   - 会直接报 data race，帮助你体会“看起来没出错，但其实是 UB”。

> 小结：
> - 计数器这种“只关心最终计数，不关心顺序”的场景，用 `atomic + relaxed` 非常合适。
> - 不需要 mutex，也不会 data race。

---

### 实践二：relaxed vs acquire-release 的对比

目标：实现一个“标志位 + 数据”的例子，分别用：

- `relaxed`（错误示范）；
- `release/acquire`（正确示范）；

感受“有无同步”在 TSan 和实际行为上的差别。

#### 错误示范：全部 relaxed（可能读到未初始化的值）

```cpp
#include <atomic>
#include <thread>
#include <iostream>

struct Data { int a; int b; };

Data g_data;
std::atomic<bool> ready_relaxed{false};

void producer_relaxed() {
    g_data.a = 1;
    g_data.b = 2;
    ready_relaxed.store(true, std::memory_order_relaxed);
}

void consumer_relaxed() {
    while (!ready_relaxed.load(std::memory_order_relaxed)) {
        // 忙等
    }
    // 这里对 g_data.a/g_data.b 的可见性是没有保证的
    std::cout << "relaxed: a=" << g_data.a << " b=" << g_data.b << "\n";
}
```

- 在某些平台/优化等级下，consumer 可能在看到 `ready_relaxed == true` 后仍旧读到旧值（甚至未初始化值）。
- TSan 也会提示存在数据竞争。

#### 正确示范：release/acquire

```cpp
std::atomic<bool> ready_ra{false};

void producer_ra() {
    g_data.a = 1;
    g_data.b = 2;
    ready_ra.store(true, std::memory_order_release);
}

void consumer_ra() {
    while (!ready_ra.load(std::memory_order_acquire)) {
        // 忙等
    }
    std::cout << "acq_rel: a=" << g_data.a << " b=" << g_data.b << "\n";
}
```

- 使用 `release/acquire` 后，读到 `ready_ra == true` 时，`g_data` 的写入对 consumer 保证可见。

#### 配合 TSan 的建议

- 用 TSan 运行 relaxed 版本，观察：
  - 对 `g_data` 的访问会被标记为 data race（因为它是普通变量，且跨线程读写无同步）。
- 用 TSan 运行 release/acquire 版本：
  - 对 `g_data` 的数据仍然不是原子访问，但通过 `ready_ra` 建立了同步关系，TSan 会把这些访问看成由原子变量同步的“有序访问”，不会报 race（视具体实现）。

> 重点：
> - `atomic` 只能保证自身那一个变量的原子性；
> - 要让“写数据 + 标志位”这类模式正确工作，必须用 release/acquire 或者更强的内存序来同步。

---

### 小结与练习

- 本日核心要点：
  - `std::atomic` 适合单变量同步；复杂对象仍需要 `mutex` 或其他协议。
  - 三种典型内存序模式：
    - `relaxed`：计数器/统计，只要原子，不要顺序；
    - `release/acquire`：发布数据 + 标志位的经典同步模式；
    - `seq_cst`：默认、语义最强，推理最简单，入门时可以全用。
  - happens-before：`store(release)` 之前的写，对 `load(acquire)` 之后的读可见。

- 建议练习：
  1. 在现有 Day8 的生产者-消费者队列基础上，尝试使用 `std::atomic<bool>` 做一个“停止标志”，体会 `seq_cst` 与 `release/acquire` 的使用差别。
  2. 自己写一个“读多写少”的标志位示例，对比：
     - 全用默认 `seq_cst`；
     - 写方 `release`、读方 `acquire`；
     - 全 `relaxed`（故意错误），配合 TSan 看差异。
  3. 尝试用 `std::atomic<long long>` 替换 Day8 中用 `mutex` 的计数器，比较代码简洁度与可读性。
