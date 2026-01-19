下面我按“知识点模块”帮你整理一批**多线程经典面试问题 + 参考详解**。你可以直接拿这些当自测题，也可以当面试口述稿素材。

---

## 一、基础概念类

### Q1：什么是线程？和进程有什么区别？

**要点回答：**

- **进程（process）**  
  - 操作系统分配资源的基本单位  
  - 拥有**独立虚拟地址空间**、打开的文件描述符、句柄等
- **线程（thread）**  
  - 进程内的执行单元  
  - 同一进程内多个线程**共享**：
    - 代码段、全局变量、堆
    - 文件/Socket 句柄等
  - 各自拥有独立栈、寄存器、程序计数器

**为什么要多线程？**

- 利用多核并行
- 隐藏 I/O 等待（一个线程 I/O，另一个线程干活）
- 响应式 UI / 后台任务

面试官常追问：**“共享什么，不共享什么”**，你要能说清：

- 共享：代码、全局/静态、堆、打开的文件句柄
- 不共享：栈、寄存器、线程本地存储

---

### Q2：什么是数据竞争（data race）？为什么危险？

**要点回答：**

- 定义（C++ 内存模型）：
  - 至少两个线程
  - 访问**同一内存位置**
  - 至少有一个是写
  - 且访问之间**没有同步关系**（没有 mutex/atomic 建立 happens-before）
- 结果：
  - **未定义行为（UB）**：不是“结果随机”，而是编译器可任意优化
  - 表现：偶现 bug、错行结果、死循环、崩溃

**典型例子：**

```cpp
int x = 0;

void t1() { x = 1; }
void t2() { std::cout << x; } // 无锁访问

// t1 和 t2 并发执行 → data race
```

**如何避免？**

- 只读共享
- 用 `std::mutex` / `std::atomic` 建立同步

---

### Q3：C++ 标准线程 API 里，`std::thread` 的生命周期和 `join/detach` 有什么要注意的？

**要点回答：**

- 构造 `std::thread` 即创建 OS 线程并启动。
- 在 `std::thread` 对象销毁前，**必须**：
  - `join()` 等待子线程结束，或
  - `detach()` 让其后台运行
- 如果在 `joinable()==true` 的情况下销毁 `std::thread`，会调用 `std::terminate`，程序直接终止。
- `detach` 慎用：
  - 线程变成“孤儿线程”，结束后资源由系统回收
  - 但你**无法再管理它的生命周期**，也很难安全访问共享数据（容易 data race / use-after-free）

---

## 二、互斥锁 / 条件变量

### Q4：什么是临界区？如何用 `std::mutex` 保护临界区？

**要点回答：**

- **临界区**：访问**共享可变状态**（全局变量、共享对象）的一段代码。
- 为防止数据竞争，需要保证**同一时刻最多一个线程**在执行这段代码。
- 用 `std::mutex` + RAII：

```cpp
std::mutex m;
int shared = 0;

void f() {
    std::lock_guard<std::mutex> lk(m); // 加锁
    ++shared;                          // 临界区
}                                      // 出作用域自动解锁
```

**面试补充：**

- 推荐 `std::lock_guard`/`std::unique_lock`，避免忘记解锁 / 异常路径漏解锁。

---

### Q5：`lock_guard` 和 `unique_lock` 区别？什么时候用哪个？

**要点回答：**

- `std::lock_guard<Mutex>`：  
  - 轻量、简单，构造时锁、析构时解锁
  - **不能**手动 unlock / 重新 lock
- `std::unique_lock<Mutex>`：  
  - 功能更强：支持
    - 延迟加锁 `std::defer_lock`
    - 手动 `lock()/unlock()`
    - 转移所有权（可移动）
    - 与 `std::condition_variable::wait` 配合（必须 `unique_lock`）

**选择：**

- 只需要“作用域锁”的简单场景 → `lock_guard`
- 需要条件变量 / 提前解锁 / 可选加锁 → `unique_lock`

---

### Q6：条件变量 `std::condition_variable` 的典型使用模式？

**要点回答：**

- 用于在“某个共享状态改变之前，阻塞等待”的场景（生产者-消费者）。
- 标准模式：

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;
int data = 0;

void producer() {
    {
        std::lock_guard<std::mutex> lk(m);
        data = 42;
        ready = true;
    }
    cv.notify_one();
}

void consumer() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return ready; }); // 防止虚假唤醒
    int x = data;
}
```

关键点：

- **必须用 `unique_lock`**，不能用 `lock_guard`。
- 必须用 **循环或谓词** 处理“虚假唤醒”：
  - 推荐 `cv.wait(lk, []{ return 条件; })`。

---

## 三、死锁相关

（你已经记了，但面试常考）

### Q7：死锁产生的条件？如何避免多锁死锁？

**要点回答：**

- 四要素：互斥、保持并等待、不可剥夺、循环等待。
- 多锁最常见死锁：加锁顺序不一致（A→B vs B→A）。
- 避免：
  - **全局加锁顺序**：例如所有地方都确保 `A < B < C` 的顺序拿锁。
  - 或使用 `std::lock(m1, m2)` / `std::scoped_lock(m1, m2)` 一次性拿多锁。
  - 减少锁的数量；锁内逻辑尽量短；不要在锁内做 I/O 或调用未知回调。

---

## 四、`std::atomic` / 内存模型

### Q8：`std::atomic` 和 `mutex` 的区别？什么时候用哪一个？

**要点回答：**

- 相同点：
  - 都可以用于多线程同步。
- 不同点：
  - `atomic<T>` 只保证**单个变量**上的原子读写/修改，无锁或轻量锁实现。
  - `mutex` 保护的是**临界区里的任意多条操作、多块数据**，提供强不变量保证。
- 适用场景：
  - `atomic`：计数器、标志位、简单指针交换、状态机小状态
  - `mutex`：复杂对象（多个字段）、容器的结构修改、需要事务性更新

一句总结：  
**“一个变量小操作 → atomic；多变量复杂不变量 → mutex。”**

---

### Q9：什么是 `memory_order_relaxed`？什么场景可以用？

**要点回答：**

- `relaxed`：只保证这次原子操作的原子性，不对跨线程顺序和可见性做额外约束。
- 适用场景：
  - 纯统计（计数器）
  - Debug 计数、性能计数，不依赖“先后”关系
- 不能用于：
  - 通过它来同步其他变量的写入／读取（那需要 `release/acquire` 或更强）

示例：计数器累加：

```cpp
std::atomic<long long> counter{0};

void worker() {
    for (...) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

---

### Q10：如何用 `release`/`acquire` 模式实现“线程间发布数据”？

**要点回答：**

- 经典“生产者写数据 + ready 标志；消费者等 ready 后读数据”。

```cpp
struct Data { int x; int y; };

Data g;
std::atomic<bool> ready{false};

void producer() {
    g.x = 1;
    g.y = 2;
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) {}
    int a = g.x; // 一定看到 1
    int b = g.y; // 一定看到 2
}
```

- `store(release)` 保证：之前的写不会被重排到它后面。
- `load(acquire)` 保证：之后的读不会被重排到它前面。
- 两者之间建立 happens-before：看到 `ready==true` 就能看到对应数据。

---

### Q11：什么是 `memory_order_seq_cst`？什么时候直接用它？

**要点回答：**

- `seq_cst` = sequentially consistent，顺序一致性。
- 直观理解：**所有线程看到的所有原子操作，好像严格按某个全局顺序执行**。
- 优点：
  - 语义最简单，推理最安全。
- 缺点：
  - 在某些平台上比 acquire/release 稍慢一点（实现需要更多栅栏）。
- 实战建议：
  - 不确定用啥，就用默认的 `seq_cst`。正确优先，性能优化以后再说。

---

## 五、设计与工程实践类

### Q12：多线程下如何设计一个“线程安全的队列”？

**要点回答（互斥 + 条件变量版本）：**

- 使用 `std::mutex` + `std::condition_variable` + `std::queue<T>`。
- 典型接口：

```cpp
template<typename T>
class ThreadSafeQueue {
public:
    void push(T value) {
        {
            std::lock_guard<std::mutex> lk(m_);
            q_.push(std::move(value));
        }
        cv_.notify_one();
    }

    T pop() {
        std::unique_lock<std::mutex> lk(m_);
        cv_.wait(lk, [this]{ return !q_.empty(); });
        T value = std::move(q_.front());
        q_.pop();
        return value;
    }

private:
    std::mutex m_;
    std::condition_variable cv_;
    std::queue<T> q_;
};
```

- 说明：
  - `push` 在持锁状态下修改队列，然后 `notify_one`。
  - `pop` 在队列为空时阻塞等待，避免忙等。
  - 锁粒度较粗但简单、正确。

面试进阶可谈：

- 支持 `try_pop` / 超时版本
- 是否需要“关闭”队列的机制（停止所有消费者）

---

### Q13：什么是“伪共享”（false sharing）？在多线程中为什么重要？

**要点回答：**

- CPU 缓存以“缓存行”（cache line）为基本单位（常见 64 字节）。
- 如果两个线程频繁写入**不同变量**，但刚好这两个变量**落在同一个 cache line** 上：
  - 即使它们逻辑上互不相关，CPU 仍然要在各核心间不停同步这条缓存行
  - 导致严重的性能下降，这就是“伪共享”
- 典型场景：多线程计数器数组 `int counters[NUM_THREADS];`，每线程写自己那一格，却性能很差。

**解决：**

- 使用 padding / 对齐，保证不同线程写的变量分布在不同 cache line：

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<long long> value{0};
};

PaddedCounter counters[NUM_THREADS];
```

---

### Q14：什么是线程安全？什么是可重入（reentrant）？区别？

**要点回答：**

- **线程安全（thread-safe）**：
  - 多个线程**并发调用**某函数或操作时，不会产生 data race / UB，结果正确。
  - 实现手段：内部加锁/原子/无锁算法等。
- **可重入（reentrant）**：
  - 函数在被**中断（例如信号）并再次（甚至递归）调用**的情况下仍表现正确。
  - 不依赖共享可变状态、只用栈/参数本地变量；不使用不可重入 API（例如某些 C 函数的静态缓存）。
- 区别：
  - 可重入通常意味着线程安全，但线程安全不一定可重入（比如内部用了静态变量和 mutex 的函数）。

---

### Q15：如何在多线程环境中安全地使用 `std::cout` / 日志？

**要点回答：**

- `std::cout` 本身不是线程安全的，多线程直接写会产生交织输出。
- 解决思路：
  - 用 **单独的日志线程 + 线程安全队列**，其他线程把日志推到队列中；
  - 或者在每次写前后**用 mutex 包裹**，但要注意锁粒度和性能。
- 示例（简单版）：

```cpp
std::mutex log_m;

void log(const std::string& msg) {
    std::lock_guard<std::mutex> lk(log_m);
    std::cout << msg << std::endl;
}
```

---

## 六、综合思路类问题

### Q16：设计一个定时任务调度器（支持多线程执行），你会考虑哪些并发问题？

可以从这几个角度回答：

- **数据结构**：任务队列（按时间排序的最小堆 / 优先队列）
- **多线程访问**：
  - 队列需要 mutex 保护
  - 新任务插入时唤醒调度线程（`condition_variable`）
- **调度线程逻辑**：
  - 拿锁查看最早任务时间
  - 计算需要等待多久，使用 `cv.wait_until` 或 `wait_for`
  - 到时间后取出任务，解锁，在线程池或工作线程中异步执行
- **问题点**：
  - 避免死锁（队列锁内不做长计算）
  - 考虑关闭 / 停止时的同步：设置 `stop` 标志，`notify_all()`，确保所有线程安全退出
  - 避免虚假唤醒（用带谓词的 wait）

回答时不一定要写代码，但**讲清楚锁 + 条件变量 + 退出逻辑**就很好。

---

## 使用建议

你可以这样用这些题：

- 先**自己口述回答一遍**（可以录音听一下有没有卡顿）。
- 再把你不太确定的几个问题标出来，我可以帮你：
  - 精简成**面试版答案**（30 秒~1 分钟）
  - 或给你补几道代码级别的小题，比如“判断这段代码是否有 data race / 死锁 / 使用锁是否正确”。

如果你愿意，下一步我也可以帮你：  
- 针对这些问题出一份**多线程专场选择题 + 简答题试卷**，跟你之前 week1 那种风格类似，方便你系统自测。