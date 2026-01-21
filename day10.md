## Day10：async / future / packaged_task / call_once

### 理论速览

- 任务模型（task-based concurrency）：
  - 从“管理线程”转向“提交任务”：不再手动创建/管理 `std::thread`，而是提交一个任务，由库/runtime 安排在线程上运行。
  - C++11 提供的基础设施：`std::async`、`std::future`、`std::packaged_task`、`std::promise` 等。

- `std::future` / `std::promise`：
  - `future<T>`：表示“**未来某个时刻才会得到的 T**”，支持阻塞等待、轮询等。
  - `promise<T>`：用于在某个线程里“**把结果写入**”，与 `future` 配对使用。

- `std::async`：
  - 提交一个任务，返回 `future`，调用 `get()` 时得到返回值。
  - 任务可以在新线程、线程池或同步方式下执行，取决于 `policy`（`launch::async` / `launch::deferred`）。

- `std::packaged_task`：
  - 把一个**可调用对象**包装成“任务 + future”的形式，可以手动安排任务在哪个线程执行。
  - 与线程池、任务队列组合时常用。

- `std::call_once` / `std::once_flag`：
  - 保证某个初始化逻辑在多线程环境下**只执行一次**，常用于实现线程安全的单例/懒初始化。

---

### std::future 与 promise 基础

#### 1. `future`：从异步任务中取结果

- `std::future<T>` 支持：
  - `T get()`：阻塞直到结果就绪，取得结果（只能调用一次）；
  - `wait()` / `wait_for()` / `wait_until()`：等待结果就绪但不取值。

简单示例：

```cpp
#include <future>
#include <iostream>

int compute() {
    // 模拟计算
    return 42;
}

int main() {
    std::future<int> f = std::async(std::launch::async, compute);
    // 这里可以做点别的事情
    int result = f.get(); // 阻塞直到 compute 完成
    std::cout << "result = " << result << "\n";
}
```

#### 2. `promise`：手动设置 future 的结果

```cpp
#include <future>
#include <thread>
#include <iostream>

void worker(std::promise<int> p) {
    try {
        int value = 42; // 计算结果
        p.set_value(value);
    } catch (...) {
        p.set_exception(std::current_exception());
    }
}

int main() {
    std::promise<int> p;
    std::future<int> f = p.get_future();
    std::thread t(worker, std::move(p));

    int result = f.get();
    std::cout << "result = " << result << "\n";

    t.join();
}
```

- `promise` + `future` 提供了一个“**跨线程传递返回值/异常**”的通道。

---

### std::async：任务级并发

#### 1. 基本用法

- `std::async` 接口：

```cpp
template<class F, class... Args>
std::future<std::invoke_result_t<F, Args...>>
async(std::launch policy, F&& f, Args&&... args);
```

- 常用 `policy`：
  - `std::launch::async`：一定在**新线程**中异步执行；
  - `std::launch::deferred`：推迟执行，直到调用 `get()` / `wait()` 才在当前线程执行；
  - 不传 `policy`：由实现自由决定（可能 async 或 deferred）。

```cpp
#include <future>
#include <iostream>

int slow_add(int a, int b) {
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    return a + b;
}

int main() {
    auto f1 = std::async(std::launch::async, slow_add, 1, 2);
    auto f2 = std::async(std::launch::async, slow_add, 3, 4);

    int r1 = f1.get();
    int r2 = f2.get();
    std::cout << r1 + r2 << "\n"; // 结果 10
}
```

> 实战建议：
> - 如果希望**明确并发执行**，优先加上 `std::launch::async`。
> - 如果允许实现选择（比如考虑线程数/资源），可以不指定，但行为会依赖标准库实现。

---

### std::packaged_task：把函数包装成“任务 + future”

- `std::packaged_task<R(Args...)>`：
  - 持有一个可调用对象（函数、lambda 等），
  - 可以被当作函数调用：执行时产生结果并把结果设置到关联的 `future` 中。

示例：

```cpp
#include <future>
#include <thread>
#include <iostream>

int compute(int x) {
    return x * 2;
}

int main() {
    std::packaged_task<int(int)> task(compute);
    std::future<int> fut = task.get_future();

    std::thread t(std::move(task), 21); // 在线程中执行任务
    t.join();

    std::cout << "result = " << fut.get() << "\n"; // 42
}
```

- `packaged_task` 适合与**任务队列/线程池**结合：
  - 主线程创建 `packaged_task` + `future`；
  - 把 task 放入队列，由工作线程取出执行；
  - 主线程后续通过 `future` 拿结果。

---

### std::call_once：一次性初始化

- 用于保证某个初始化逻辑只执行一次，即使有多个线程同时尝试初始化。

接口：

```cpp
#include <mutex>

std::once_flag flag;

void init() {
    // 只执行一次的初始化逻辑
}

void worker() {
    std::call_once(flag, init);
    // 后续使用已初始化的资源
}
```

- 常用来实现线程安全的懒初始化/单例：
  - 只在第一次使用时初始化资源。
  - 后续所有线程都跳过初始化部分，直接用结果。

> 与“自己用 mutex + 静态 bool 标志”相比，`call_once` 封装了所有并发边界情况，更安全可靠。

---

### 实践一：用 async 做并行累加

目标：

- 把一个大数组/范围的累加任务拆成多块，利用 `std::async` 并行累加，再合并结果。

#### 示例代码（简化版）

```cpp
#include <vector>
#include <numeric>
#include <future>
#include <iostream>

long long partial_sum(const std::vector<int>& v, size_t begin, size_t end) {
    return std::accumulate(v.begin() + begin, v.begin() + end, 0LL);
}

int main() {
    const size_t N = 1'000'000;
    std::vector<int> data(N, 1);

    size_t mid = N / 2;

    auto f1 = std::async(std::launch::async, partial_sum, std::cref(data), 0, mid);
    auto f2 = std::async(std::launch::async, partial_sum, std::cref(data), mid, N);

    long long s1 = f1.get();
    long long s2 = f2.get();

    std::cout << "sum = " << (s1 + s2) << "\n"; // 应该是 N
}
```

练习思路：

- 把两块改成 `num_tasks` 块，用一个 vector 存 `future<long long>`，循环 `get()` 合并结果。
- 用 `std::chrono` 计时，对比单线程 `std::accumulate` 与 `async` 版本的性能（注意：小任务未必更快）。

> 关键点：
> - `async` 让你**不必手动管理线程对象**，只关心任务和结果。
> - `future::get()` 是同步点，会在结果就绪前阻塞。

---

### 实践二：用 call_once 实现线程安全单例

目标：

- 写一个简单的单例类，支持多线程安全获取实例。

#### 示例代码

```cpp
#include <mutex>
#include <iostream>

class Singleton {
public:
    static Singleton& instance() {
        std::call_once(init_flag_, [] {
            instance_ = new Singleton();
        });
        return *instance_;
    }

    void do_something() {
        std::cout << "Singleton::do_something\n";
    }

private:
    Singleton() = default;
    ~Singleton() = default;

    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;

    static std::once_flag init_flag_;
    static Singleton* instance_;
};

std::once_flag Singleton::init_flag_;
Singleton* Singleton::instance_ = nullptr;

int main() {
    auto worker = [] {
        Singleton::instance().do_something();
    };

    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();
}
```

说明：

- `std::call_once` 确保 `instance_ = new Singleton();` 只执行一次；
- 多个线程并发第一次调用 `instance()` 也不会竞争初始化；
- 析构与资源释放在真实项目中可能要更精细处理（比如用 `std::unique_ptr` + `atexit`）。

> 实战：
> - C++11 起也可以依赖“**函数内静态局部变量初始化的线程安全性**”实现更简洁的单例：
>
>   ```cpp
>   class Singleton {
>   public:
>       static Singleton& instance() {
>           static Singleton inst; // C++11 保证线程安全初始化
>           return inst;
>       }
>   };
>   ```
>
> - 但 `call_once` 思路依然很重要，很多库在更复杂场景下会使用。

---

### 小结与练习

- 本日核心要点：
  - `std::future` / `std::promise`：跨线程传递返回值与异常。
  - `std::async`：用任务模型提交异步任务，通过 `future` 获取结果。
  - `std::packaged_task`：把可调用对象包装成“任务+future”，方便与线程池/任务队列结合。
  - `std::call_once` / `std::once_flag`：实现一次性初始化，典型例子是线程安全单例。

- 建议练习：
  1. 把 Day8 的生产者-消费者队列改造为“任务队列 + worker 线程”，用 `packaged_task` 封装任务，并返回 `future` 获取结果。
  2. 改写并行累加示例：用 `packaged_task` + `std::thread` 手工实现一版，对比 `std::async` 的代码简洁度。
  3. 用 `std::call_once` 为某个全局日志系统做懒初始化，保证日志文件只被打开一次，并从多个线程中写入测试其行为。
