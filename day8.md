## Day8：并发基础（C++11 为主）

### 理论速览

- 并发与线程模型：
  - 线程（`std::thread`）：进程内的执行单元，多个线程共享同一进程的地址空间。
  - 数据竞争（data race）：多个线程在缺少同步的情况下同时读写同一内存位置，行为未定义，常表现为“偶现 bug”。
- 互斥与临界区：
  - 互斥锁（`std::mutex`）：保证同一时刻只有一个线程可以进入某段代码（临界区）。
  - RAII 加锁：用 `std::lock_guard`/`std::unique_lock` 自动管理 `lock/unlock`，避免忘记解锁或异常路径泄露锁。
- 条件变量（`std::condition_variable`）：
  - 用于线程间的“等待某个条件成立 / 条件满足后唤醒”的场景。
  - 典型模式：生产者-消费者、有界缓冲区、任务队列。
- 死锁与顺序加锁：
  - 死锁：多个线程互相持有对方需要的锁，并且都在等待对方释放，所有线程都无法继续。
  - 顺序加锁原则：对多把锁规定一个全局顺序（如 `A < B < C`），所有地方都必须按同一顺序获取锁，从而避免循环等待。

---

### std::thread 基础

#### 1. 创建与 join

- 通过 `std::thread` 启动新线程：
  - 构造时传入函数和参数，线程立即开始执行。
  - 主线程需要在合适的时机调用 `join()` 等待子线程结束，或调用 `detach()` 让其在后台自行运行（慎用）。

```cpp
#include <thread>
#include <iostream>

void worker(int id) {
    std::cout << "worker " << id << " running\n";
}

int main() {
    std::thread t1(worker, 1);  // 启动线程，执行 worker(1)

    std::thread t2([] {
        std::cout << "lambda worker running\n";
    });

    t1.join();  // 等待 t1 结束
    t2.join();  // 等待 t2 结束
}
```

#### 2. 按引用传参

- 线程函数参数默认按值拷贝传入，如需按引用传递，要用 `std::ref` 包一层：

```cpp
#include <thread>
#include <iostream>

void inc(int &x) {
    ++x;
}

int main() {
    int value = 0;
    std::thread t(inc, std::ref(value)); // 按引用传递参数
    t.join();
    std::cout << "value = " << value << "\n";  // 输出 1
}
```

> 习惯：只在确实需要共享可变状态时用引用参数，否则优先值传递，减少共享数据。

---

### 互斥锁：std::mutex / lock_guard / unique_lock

#### 1. 用 mutex + lock_guard 保护共享数据

- 典型场景：多个线程并发累加一个全局计数器，如果不加锁会产生数据竞争。

```cpp
#include <mutex>
#include <thread>
#include <vector>
#include <iostream>

int counter = 0;            // 共享数据
std::mutex counter_mtx;     // 对应的互斥锁

void add() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lock(counter_mtx); // 构造时加锁，析构时解锁
        ++counter;
    }
}

int main() {
    std::thread t1(add);
    std::thread t2(add);

    t1.join();
    t2.join();

    std::cout << "counter = " << counter << "\n";
}
```

- `std::lock_guard`：
  - 构造函数中调用 `mtx.lock()`；
  - 析构函数中调用 `mtx.unlock()`；
  - 适用于“简单的作用域加锁”，不能手动 `lock/unlock`，也不能用于条件变量的等待。

#### 2. unique_lock 的灵活用法

- `std::unique_lock` 提供更灵活的锁管理：
  - 支持延迟加锁、提前解锁；
  - 支持手动 `lock()/unlock()`；
  - 可以配合 `std::condition_variable` 使用（`wait` 需要 `unique_lock`）。

```cpp
#include <mutex>

std::mutex m;

void foo() {
    std::unique_lock<std::mutex> lk(m); // 构造时加锁
    // ... 临界区
    lk.unlock();                         // 提前解锁
    // ... 非临界区
    lk.lock();                           // 需要时再加锁
}
```

> 实战建议：
> - 不需要条件变量时，优先用简单的 `std::lock_guard`。
> - 使用 `std::condition_variable` 时，使用 `std::unique_lock`。

---

### 条件变量：std::condition_variable

#### 1. 典型用途

- 线程 A 在某个条件不满足时等待（不忙轮询），线程 B 修改共享状态并唤醒 A：
  - 生产者-消费者；
  - 任务队列 / 线程池；
  - 有界缓冲区（缓冲区满时阻塞生产者、为空时阻塞消费者）。

#### 2. 使用模式

- 常见模式：
  - 持有 `std::unique_lock<std::mutex>`；
  - 在循环中调用 `cv.wait(lock, predicate)`；
  - 被唤醒后重新检查条件。

伪代码：

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

void waiting_thread() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return ready; }); // ready 为 true 时返回
    // ... 条件满足后的处理
}

void notifying_thread() {
    {
        std::lock_guard<std::mutex> lk(m);
        ready = true;
    }
    cv.notify_one(); // 或 notify_all()
}
```

> 重要：使用 `wait(lock, pred)` 而不是手写 while+wait，可以避免假唤醒带来的错误。

---

### 实践：生产者-消费者队列（条件变量）

目标：用 `std::mutex + std::condition_variable` 实现一个简单的生产者-消费者模型：

- 一个线程不断生产整数，放入队列；
- 多个消费者线程从队列中取出数据并处理；
- 当生产结束且队列为空时，消费者线程自然退出。

#### 1. 简化实现示例

```cpp
#include <queue>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <iostream>
#include <chrono>

std::queue<int> q;
std::mutex q_mtx;
std::condition_variable q_cv;
bool done = false; // 标记生产是否结束

void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard<std::mutex> lk(q_mtx);
            q.push(i);
            std::cout << "produce " << i << "\n";
        } // lk 析构，释放锁

        q_cv.notify_one(); // 通知一个消费者队列有新数据

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    {
        std::lock_guard<std::mutex> lk(q_mtx);
        done = true;
    }
    q_cv.notify_all(); // 通知所有消费者：不会再有新数据
}

void consumer(int id) {
    while (true) {
        std::unique_lock<std::mutex> lk(q_mtx);
        q_cv.wait(lk, [] { return !q.empty() || done; });

        if (!q.empty()) {
            int value = q.front();
            q.pop();
            lk.unlock(); // 解锁再处理，缩小临界区

            std::cout << "consumer " << id << " got " << value << "\n";
        } else if (done) {
            // 队列为空且生产结束，可以退出
            break;
        }
    }
}

int main() {
    std::thread p(producer);
    std::thread c1(consumer, 1);
    std::thread c2(consumer, 2);

    p.join();
    c1.join();
    c2.join();
}
```

#### 2. 关键点小结

- 队列 `q` 和标志 `done` 都是共享数据，必须在持有 `q_mtx` 的情况下访问。
- `wait(lk, pred)` 会在等待时自动释放锁，并在被唤醒后重新加锁，再检查条件。
- 处理元素尽量在解锁后执行，减小临界区长度，提高并发度。

> 练习扩展：为队列增加“最大容量”，当队列满时让生产者等待，从而实现有界缓冲区。

---

### 死锁与顺序加锁

#### 1. 死锁的常见模式

- 线程 A：先锁 `m1`，再锁 `m2`；
- 线程 B：先锁 `m2`，再锁 `m1`；
- 如果 A 获得了 `m1` 而等待 `m2`，同时 B 获得了 `m2` 而等待 `m1`，双方都无法继续执行 → 死锁。

#### 2. 顺序加锁原则

- 思路：对所有需要组合使用的锁规定一个固定顺序，比如：
  - `db_mtx` < `log_mtx` < `cache_mtx`；
- 要求所有代码都按这个顺序获取锁：

```cpp
std::mutex m1, m2;

void f() {
    std::lock_guard<std::mutex> lk1(m1);
    std::lock_guard<std::mutex> lk2(m2);
    // ...
}

void g() {
    std::lock_guard<std::mutex> lk1(m1);
    std::lock_guard<std::mutex> lk2(m2);
    // ...
}
```

- 只要没有任何地方违反顺序（比如先锁 `m2` 再锁 `m1`），就不会出现上面的循环等待。

> 实战建议：
> - 在文档或注释中明确列出全局锁的获取顺序；
> - 代码评审时重点检查是否有人违反该顺序；
> - 尽量减少“同时需要多把锁”的设计，降低死锁风险。

---

### 小结与练习

- 本日重点：
  - 会使用 `std::thread` 启动线程并正确 `join`；
  - 理解数据竞争，并能用 `std::mutex` + RAII 锁保护共享数据；
  - 会写一个基于 `std::condition_variable` 的生产者-消费者队列；
  - 知道死锁的成因，并能用顺序加锁策略规避。

- 建议练习：
  - 自己实现一个“有界队列”：队列有最大容量，上限时阻塞生产者，下限时阻塞消费者。
  - 写一个多线程计数器基准，对比：不加锁 / 用 `std::mutex` / 用 `std::atomic<int>` 的正确性与性能差异。
  - 设计一段会死锁的示例代码，再按照顺序加锁原则改写，验证死锁消失。
