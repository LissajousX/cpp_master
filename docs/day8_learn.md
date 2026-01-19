# 线程和数据竞争
## 一、线程与并发模型：从“单线”到“多线”

- **进程 vs 线程**  
  - **进程**：操作系统分配资源的基本单位，有自己独立的虚拟地址空间。  
  - **线程**：进程内的执行单元，同一进程内的线程**共享**：
    - 代码段、全局变量、堆
    - 文件描述符、socket 等资源
  - **不共享**的通常是：
    - 栈（每个线程一块独立栈）
    - 寄存器（程序计数器、通用寄存器）

- **`std::thread` 做的事情**  
  - 把一个可调用对象（函数/函数对象/lambda）交给它：
    ```cpp
    void worker(int id) {
        // do work
    }

    int main() {
        std::thread t(worker, 1);  // 创建并启动线程
        t.join();                  // 等待线程结束
    }
    ```
  - 关键点：
    - **创建即启动**：构造 `std::thread` 就创建 OS 线程并开始执行。
    - 必须在析构前 `join()` 或 `detach()`，否则程序终止（`std::terminate`）。
    - 线程函数里访问的全局/堆对象，默认是**共享的**，这就是后面数据竞争问题的根源。

---

## 二、数据竞争（data race）：为什么是“未定义行为”

### 1. 数据竞争的典型定义

在 C++ 内存模型中，**数据竞争 = 未定义行为（UB）**。  
满足下面条件同时发生，就是 data race：

- 至少两个线程
- 访问**同一个内存位置**（同一个变量或对象的同一部分）
- 至少有一个是**写操作**
- 这两次访问之间**没有同步关系**（没有通过 `mutex`、`atomic` 等建立“发生在先”（happens-before））

常见例子：

```cpp
int x = 0;

void t1() {
    x = 1;
}

void t2() {
    std::cout << x << std::endl;
}

int main() {
    std::thread a(t1);
    std::thread b(t2);
    a.join();
    b.join();
}
```

这里 `t1` 和 `t2` 对 `x` 访问没有同步，属于**数据竞争**，行为未定义：
- 可能打印 0、1
- 也可能出现奇怪结果（在更复杂类型下）
- 甚至在极端情况下崩溃（编译器优化 + CPU 重排序）

### 2. 为什么是“未定义”，而不是“结果随机”

关键点：未定义行为不是“结果随机”，而是——**编译器可以假设这种情况永远不会发生**，所以可以做非常激进的优化。

例如：

```cpp
// 假设 x 是普通 int，全局变量
while (x == 0) {
    // do something
}
```

如果当前线程中**没有任何代码写 x**，编译器会认为 `x` 永远为 0 或永远不变：
- 可能把循环优化为死循环
- 或把对 `x` 的读取缓存到寄存器，不再从内存读

另一个线程对 `x` 的写入，在没有同步的前提下，**对这个线程来说就是“不可见”的”**，行为不受标准约束。

---

## 三、数据竞争 vs 合法的“并发访问”

有些多线程访问共享数据是**合法的**，区别在于用没用同步：

1. **只读共享是安全的**  
   - 多个线程同时**只读**同一个对象，且对象在此期间**不会被写**，是安全的。
   - 例如：
     ```cpp
     const std::string config = load_config_once();

     void worker() {
         // 多线程只读 config
         use(config);
     }
     ```
   - 前提：`config` 在多线程开始前构造完毕，之后不再修改。

2. **通过互斥量 `std::mutex` 保护读写**

   ```cpp
   int x = 0;
   std::mutex m;

   void t1() {
       std::lock_guard<std::mutex> lk(m);
       x = 1;
   }

   void t2() {
       std::lock_guard<std::mutex> lk(m);
       std::cout << x << std::endl;
   }
   ```

   - 上锁后，`x` 的访问在同一时刻只发生在一个线程中。
   - `mutex` 同时建立了**happens-before 关系**：释放锁的写，对随后获得同一把锁的读是可见的。

3. **用 `std::atomic` 做无锁计数器等**

   ```cpp
   std::atomic<int> counter{0};

   void inc() {
       counter.fetch_add(1, std::memory_order_relaxed);
   }
   ```

   - 对 `counter` 的访问是**原子的**，没有 data race。
   - 即使多个线程同时 `inc()`，最终结果仍合法（但具体次序不保证）。
   - memory order 决定“可见性”和“重排序约束”，但**只要是 atomic，就不会是 data race**。

---

## 四、典型“偶现 bug”场景

- **竞态初始化**：多个线程并发初始化某个全局/单例对象，没有加锁或使用 `std::call_once`。
- **vector 扩容时跨线程访问**：
  ```cpp
  std::vector<int> v;

  void writer() {
      for (int i = 0; i < 100000; ++i)
          v.push_back(i);  // 修改 v
  }

  void reader() {
      for (;;) {
          if (!v.empty())
              int x = v[0]; // 与 writer 同时访问 v
      }
  }
  ```
  - `push_back` 可能触发重新分配，`reader` 持有的指针/引用失效，完全 UB。
- **多线程写同一 `std::string` 或 `std::map` 而不加锁**：  
  - C++ 容器，除非特别注明（如某些并发容器），**都不是线程安全的**。

---

## 五、面试常见进一步追问点（你可以提前准备）

- **如何避免数据竞争？**  
  - **设计层面**：  
    - 尽量使用**线程内私有数据**，减少共享。  
    - 不可避免共享时，考虑**“只读共享 + 不变量”**，把修改集中在单线程。
  - **技术手段**：  
    - `std::mutex / std::lock_guard / std::unique_lock`  
    - `std::atomic`（计数器、标志位、队列索引等）
    - 高层抽象（任务队列、消息队列、actor 模型）

- **`mutex` vs `atomic` 使用场景**  
  - `atomic`：
    - 适合简单标志/计数/指针等**单变量**操作
    - 无锁、低延迟，但写多读多时有 cache line 抖动/伪共享问题
  - `mutex`：
    - 适合保护**复合数据结构**（如 `std::map`、多个字段组成的状态）
    - 写操作临界区较大时，更直观安全

- **为什么 `volatile` 不能用来做线程同步？**  
  - `volatile` 只保证编译器不优化掉对内存的读写，但**不保证 CPU/cache 层面的可见性与顺序**。
  - C++ 并发依赖的是 `atomic` + 内存序，或 `mutex` 等同步原语。

---

## 小结（面试用的一句话版本）

- **线程**：进程内的执行单元，多个线程共享地址空间与大部分资源。
- **数据竞争**：多个线程在**无同步**的情况下，对同一内存位置进行**至少一个写**访问，属于**未定义行为**。  
- **应对策略**：  
  - 减少共享  
  - “只读共享 + 不变量”  
  - 写时加锁（`mutex`）或使用 `atomic` 建立合法的并发访问与可见性关系。


# 各种锁

# C++ 里的各种“锁”：概念 + 例子

我分三层给你讲：

- **基础锁原语**：`std::mutex`、`recursive_mutex`、`timed_mutex`
- **读写/自旋/原子**：`shared_mutex`、自旋锁、`std::atomic`
- **更高层概念**：RAII 加锁、乐观/悲观锁、细粒度/粗粒度、死锁

---

## 一、最常用的：互斥锁（mutex）

### 1. `std::mutex`：最基础的互斥

**概念**：同一时刻只允许一个线程进入“临界区”。

```cpp
#include <mutex>
#include <thread>
#include <iostream>

int counter = 0;
std::mutex m;

void worker() {
    for (int i = 0; i < 100000; ++i) {
        std::lock_guard<std::mutex> lk(m); // 上锁到作用域结束自动解锁
        ++counter;                          // 临界区
    }
}

int main() {
    std::thread t1(worker), t2(worker);
    t1.join(); t2.join();
    std::cout << counter << "\n"; // 理论上是 200000
}
```

要点：

- **一定用 RAII**（`lock_guard`、`unique_lock`），不要手动 `lock()/unlock()`，异常和早退很容易忘记解锁。

### 2. `std::lock_guard` vs `std::unique_lock`

- `lock_guard`：最简单 RAII，加锁即构造，离开作用域解锁，**不能提前解锁**。
- `unique_lock`：更灵活，可以延迟加锁、手动解锁、与 `condition_variable` 配合。

```cpp
std::mutex m;

// lock_guard 版
void f1() {
    std::lock_guard<std::mutex> lk(m); // 构造时加锁
    // ...
} // 离开作用域自动 unlock

// unique_lock 版
void f2() {
    std::unique_lock<std::mutex> lk(m, std::defer_lock); // 暂不加锁
    // 做点无锁的事情
    lk.lock();   // 需要时再加锁
    // ...
    lk.unlock(); // 可以提早解锁
    // ...
}
```

### 3. 递归锁：`std::recursive_mutex`

**概念**：允许**同一个线程**重复加同一把锁（计数+1/-1），常见于递归调用、成员函数相互调用。

```cpp
#include <mutex>

std::recursive_mutex rm;
int data = 0;

void f(int depth) {
    std::lock_guard<std::recursive_mutex> lk(rm);
    if (depth == 0) return;
    ++data;
    f(depth - 1); // 同一线程再次 lock，同样可以成功
}
```

注意：

- 用递归锁**是无奈之举**，更推荐重新设计，减少需要递归锁的场景。
- 不同线程如果乱用 recursive_mutex 一样会死锁。

### 4. 带超时的锁：`timed_mutex` / `recursive_timed_mutex`

允许设定**超时时间**，获取不到锁就放弃，而不是一直阻塞。

```cpp
#include <mutex>
#include <chrono>

std::timed_mutex tm;

void try_work() {
    if (tm.try_lock_for(std::chrono::milliseconds(10))) {
        // 获取到了锁
        // ...
        tm.unlock();
    } else {
        // 10ms 内没拿到锁，放弃
    }
}
```

适用场景：不想长时间挂死等待锁，需要**降级处理**或**超时提示**。

---

## 二、读写锁、自旋锁与原子

### 1. 读写锁：`std::shared_mutex` / `shared_timed_mutex`

**概念**：  
- 多个线程可以同时“**共享读锁**”  
- 写操作必须获取“**独占写锁**”，此时没有其它读/写

适用于“**读多写少**”场景。

```cpp
#include <shared_mutex>
#include <thread>
#include <vector>

std::shared_mutex sm;
std::vector<int> data;

void reader() {
    std::shared_lock<std::shared_mutex> lk(sm); // 读锁
    // 可以同时有很多 reader 拿到锁
    if (!data.empty()) {
        int x = data.back();
        // ...
    }
}

void writer() {
    std::unique_lock<std::shared_mutex> lk(sm); // 写锁，独占
    data.push_back(42);
}
```

优点：

- 读多写少时性能更好（相比所有读也用 mutex）。

陷阱：

- 写操作容易**饥饿**（一直有读锁进来，写锁很难拿到），实现可以选择不同策略，但你要**知道有这个问题**。

### 2. 自旋锁（spinlock）

标准库没直接提供自旋锁，但可以用 `std::atomic_flag` 自己写一个**极简版本**：

```cpp
#include <atomic>

class Spinlock {
    std::atomic_flag flag = ATOMIC_FLAG_INIT;

public:
    void lock() {
        while (flag.test_and_set(std::memory_order_acquire)) {
            // 忙等；可以加 pause/yield 减少 CPU 浪费
        }
    }

    void unlock() {
        flag.clear(std::memory_order_release);
    }
};
```

特点：

- **忙等**（spin）：拿不到锁就一直在循环占着 CPU。
- 适用于：
  - 临界区非常非常短
  - 持锁时间非常小
  - 线程数不多、锁竞争不太激烈的场景
- 不适用于：
  - 临界区很长
  - 可能因为 I/O / 系统调用阻塞

否则就会白白烧 CPU，不如直接 `std::mutex` 让出 CPU。

### 3. `std::atomic`：无锁（lock-free）风格

原子类型本质上是一种“**微型锁原语**”，在很多简单场景下可以**不使用 mutex**：

```cpp
#include <atomic>
#include <thread>

std::atomic<int> counter{0};

void worker() {
    for (int i = 0; i < 100000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

优点：

- 无锁，无需 mutex
- 单变量计数、标志、指针等简单场景性能很好

注意：

- 原子只能保证**单个变量**上的原子性和一定的可见性
- 复杂结构（比如 `std::map`）仍然需要 mutex 或无锁数据结构（很复杂）

---

## 三、`memory_order` 入门（最少够用版）

`std::atomic` 的所有成员函数都有一个可选的 `memory_order` 参数，用来控制**这次原子操作前后的普通读写，在不同线程眼里能否乱序 / 互相看见**。

常见枚举值有：

- `memory_order_relaxed`
- `memory_order_acquire`
- `memory_order_release`
- `memory_order_acq_rel`
- `memory_order_seq_cst`

实战里多数场景只需要记住这三种：

- **relaxed**：只要原子性，不要求跨线程顺序 / 可见性。
- **release + acquire**：在线程间建立“写在前，读在后”的 **happens-before** 关系。
- **seq_cst**：默认值，最强、最保守、最容易理解（所有原子操作看起来有一个全局总顺序）。

### 1. 计数器：`memory_order_relaxed`

用于纯统计、计数等**只关心最终数值，不关心顺序**的场景：

```cpp
std::atomic<long long> counter{0};

void worker() {
    for (int i = 0; i < 1000000; ++i)
        counter.fetch_add(1, std::memory_order_relaxed);
}
```

保证：

- 不会 data race
- 最终 `counter` 等于所有线程加和

不保证：

- 谁先谁后、看到这个值时别的普通变量一定已经更新等“时序推理”。

### 2. 标志 + 数据：`release` / `acquire`

典型模式：**一个线程写好数据再把标志设为 true，另一个线程看到标志为 true 才去读数据**。

```cpp
struct Data { int x; int y; };

Data g_data;
std::atomic<bool> ready{false};

void producer() {
    g_data.x = 42;
    g_data.y = 100;
    // 发布“数据已就绪”的标志
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) {
        // 等待就绪
    }
    // 一旦看到 ready == true，就一定能看到 x,y 的最终值
    int a = g_data.x;
    int b = g_data.y;
}
```

记忆法：

- 写线程：最后“插旗” → `store(..., release)`
- 读线程：看到旗子后再继续 → `load(..., acquire)`

这样就建立了“producer 中写 `g_data`” **happens-before** “consumer 读 `g_data`”。

### 3. 不想管细节：用默认 `seq_cst`

如果你暂时搞不清楚需要什么顺序，安全起见可以全部使用默认的 `memory_order_seq_cst`：

```cpp
std::atomic<int> x{0};

void f() {
    x.store(1);      // 等价于 x.store(1, std::memory_order_seq_cst);
    int v = x.load();
}
```

在绝大多数业务代码里，这已经足够正确，只是在某些极限性能场景下比 acquire/release 稍重一些。

小结一张“速记表”：

- **计数 / 统计**：`fetch_add(..., relaxed)`
- **写数据 + 标志 → 另一线程看到标志后读数据**：`store(release)` / `load(acquire)`
- **搞不清楚就先用**：默认 `seq_cst`

---

## 四、条件变量不是锁，但经常和锁一起出现

`std::condition_variable` 自己不是锁，但需要与 `std::unique_lock<std::mutex>` 配合，体现“**等待某个条件成立**”的场景。

典型生产者-消费者例子：

```cpp
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>

std::mutex m;
std::condition_variable cv;
std::queue<int> q;
bool done = false;

void producer() {
    for (int i = 0; i < 10; ++i) {
        {
            std::lock_guard<std::mutex> lk(m);
            q.push(i);
        }
        cv.notify_one(); // 通知一个消费者
    }
    {
        std::lock_guard<std::mutex> lk(m);
        done = true;
    }
    cv.notify_all(); // 通知所有消费者：可以结束了
}

void consumer() {
    std::unique_lock<std::mutex> lk(m);
    while (true) {
        cv.wait(lk, []{ return !q.empty() || done; }); // 条件不满足则自动释放锁并睡眠
        if (!q.empty()) {
            int x = q.front(); q.pop();
            lk.unlock();
            // 处理 x
            lk.lock();
        } else if (done) {
            break;
        }
    }
}
```

- 条件变量主要解决：**如果条件没满足，就进入睡眠等待，而不是自旋浪费 CPU**。
- 和锁的关系：加锁保护共享数据，条件变量负责“睡/醒”。

---

## 五、几个“抽象层”的概念：经常被问但不一定对应具体类型

这些在面试里也常听到：

1. **悲观锁 vs 乐观锁**
   - 悲观锁：认为别人一定会来抢，**先上锁再干事**（`mutex`、数据库行锁）。
   - 乐观锁：认为冲突不多，先干事，提交前检查有没有冲突，冲突就重试（CAS、版本号等）。

2. **粗粒度锁 vs 细粒度锁**
   - 粗粒度：大锁包住很多代码/数据结构，简单但并发度低。
   - 细粒度：对不同部分用不同锁，并发度高、逻辑复杂、易出错。

3. **可重入锁 vs 不可重入锁**
   - 可重入：同一线程可以多次获得（如 `recursive_mutex`）。
   - 不可重入：`std::mutex`，同一线程重复 `lock()` 会死锁。

4. **公平锁 vs 非公平锁**
   - 公平锁：严格按排队顺序唤醒等待线程，避免饥饿；实现复杂/性能略差。
   - 非公平锁：谁抢到是谁，可能让某些线程长期被饿死，但吞吐量高。  
   - C++ 标准库并没有规定 mutex 必须公平，大多实现是**非公平锁**。

5. **死锁与锁顺序**
   - 多把锁时容易互相等待。常用解法：
     - **固定锁顺序**：所有地方都按 A→B→C 的顺序加锁。
     - 使用 `std::scoped_lock` 或 `std::lock(m1, m2)` 在多锁上实现死锁避免。

```cpp
std::mutex m1, m2;

void f() {
    std::scoped_lock lk(m1, m2); // 一次性拿多把锁，内部防死锁
    // ...
}
```

---

## 六、小结：面试时可以怎么“打包描述”

如果你要在面试中简短地讲“锁和同步机制”，可以按这样的结构说：

- **互斥锁**：`std::mutex` + RAII（`lock_guard`/`unique_lock`），用于保护共享数据的原子修改。
- **读写锁**：`std::shared_mutex` 支持多读单写，读多写少时有性能优势。
- **自旋锁/atomic**：使用 `std::atomic` 或自旋锁实现无锁/低开销同步，但只适合短临界区和简单数据。
- **条件变量**：`std::condition_variable` + `unique_lock` 实现等待条件、生产者-消费者，不是锁但和锁一起用。
- **高阶概念**：悲观/乐观锁、细粒度锁、死锁与锁顺序、可重入锁、锁公平性。

# std::atomic
下面我按“从直觉到细节”的顺序，把 `std::atomic` 讲清楚，重点是你**能区分什么时候该用 atomic、它解决什么问题、内存序至少要怎么用到不翻车**。

---

## 1. `std::atomic` 是什么？

- 本质：**对某个变量的访问是“原子的 + 受内存模型约束的”**。
- 它解决的问题：
  - 多线程对同一个变量读写时，普通类型（`int`, `bool`, 指针, `struct` …）会产生 **data race → 未定义行为**。
  - 把这个变量改成 `std::atomic<T>`，在正确使用的前提下，**不会产生 data race**。

对比：

```cpp
int x = 0;               // 非原子，多线程 ++x 是 UB
std::atomic<int> y{0};   // 原子，多线程 y++ 是合法的
```

---

## 2. atomic 解决了什么，没解决什么？

- **解决：**
  - 对单个变量的读写/自增自减/交换 等操作是**不可被撕裂的**（不会中途被打断导致读到“半个值”）。
  - 不会产生 data race（因为标准保证 atomic 的访问有“同步关系”）。

- **没有解决：**
  - 复杂不变量，比如“`size` 跟 `vector` 内容一致”、“两个字段要么同时更新，要么都不更新”等。
  - 多字段更新仍然需要 mutex 或更复杂的无锁结构。

直觉：**atomic 是“一个变量的小锁”**，但你不能指望它搞定整个复杂对象的一致性。

---

## 3. 最常见用法：原子计数器 & 标志位

### 3.1 原子计数器

```cpp
#include <atomic>
#include <thread>

std::atomic<long long> counter{0};

void worker() {
    for (int i = 0; i < 1000000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
        // 或 ++counter;
    }
}
```

- 多线程并发调用 `worker()`，最终 `counter` 是所有线程累加的总和，不会 UB。
- `memory_order_relaxed`：只要求原子性，不要求跨线程的“顺序可见性”（适合纯计数、纯统计）。

### 3.2 原子标志位（停止信号）

```cpp
std::atomic<bool> stop{false};

void worker() {
    while (!stop.load(std::memory_order_acquire)) {
        // 做一些工作
    }
}

int main() {
    std::thread t(worker);
    // ...
    stop.store(true, std::memory_order_release); // 通知停止
    t.join();
}
```

- 写方（`store`）用 `release`，读方（`load`）用 `acquire`：
  - 保证写方在 `store` 之前的所有写，对读方在 `load` 之后是**可见**的。
- **记忆法**：“写 release，读 acquire，建立一个 happens-before”。

---

## 4. 内存序（memory_order）最低限度要会的三种

你可以先只记 3 种（够用很久）：

1. `memory_order_relaxed`
   - 只保证原子操作本身不被撕裂，**不保证跨线程的先后顺序/可见性**。
   - 适用：计数器、debug 统计、对结果不关心谁先谁后的场景。
2. `memory_order_acquire`
   - 用在 `load` / `read`：
   - 保证：该 `load` 之后的普通内存访问**不会被重排到它前面**，并且能看到对应 `release` 之前的写。
3. `memory_order_release`
   - 用在 `store` / `write`：
   - 保证：该 `store` 之前的普通内存写**不会被重排到它后面**。

组合：  
- **典型同步场景**：
  - 线程 A 写数据 → `flag.store(true, release)`
  - 线程 B `while (!flag.load(acquire))`；之后读数据  
  → B 一定能看到 A 在 `store` 之前写入的内容。

如果记不住，保底可以先用默认的 `memory_order_seq_cst`（顺序一致性，最强），等熟悉再优化。

---

## 5. `compare_exchange`：乐观锁 / CAS 基石

很多“无锁结构”是靠 CAS（compare-and-swap）实现的，对应 C++ 原子是 `compare_exchange_weak/strong`。

例子：原子地把指针从 `old` 改成 `new`，前提是当前值确实还是 `old`：

```cpp
std::atomic<int*> p{nullptr};

void set_if_null(int* newp) {
    int* expected = nullptr;
    p.compare_exchange_strong(
        expected, newp,
        std::memory_order_acq_rel,  // 成功时的 memory order
        std::memory_order_acquire   // 失败时的 memory order
    );
}
```

- 如果 `p` 目前是 `nullptr`，则原子地写入 `newp`，返回 true。
- 否则不改，返回 false，并把当前值写回 `expected`。

一般只在写**无锁队列/栈、lock-free 容器**才会深入用 CAS。面试里你能**说出它的用途**，比真写出复杂实现更重要。

---

## 6. atomic vs mutex：什么时候用哪个？

- 更适合用 **`atomic`**：
  - 简单计数器（请求数、任务数）
  - 简单标志位（停止信号、状态位）
  - 指针交换、单生产者/单消费者 ring buffer 索引

- 更适合用 **`mutex`**：
  - 多个字段需要一起更新保持不变量
  - 复杂容器（`std::map` / `std::unordered_map`）要跨线程修改
  - 临界区逻辑较复杂（多个函数调用、需要强一致性）

**口诀**：  
- “**一个变量，小操作 → atomic；多变量/复杂逻辑 → mutex**”。

---

## 7. 常见坑

- **以为 atomic 自动优化性能**：
  - atomic 本身也有开销（cache 同步、总线锁等），用多了反而慢。
- **对 struct 做 atomic 就万事大吉**：
  - `std::atomic<MyStruct>` 只有在 `MyStruct` 满足某些条件（trivially copyable）才有意义，即便如此，也只是“这个 struct 本身的读写原子”，没帮你维护内部不变量。
- **拿 `volatile` 当 atomic 用**：
  - 错！C++ 里 `volatile` 不提供并发语义，只是“别优化掉这次访问”，不能替代 atomic 或 mutex。

---

## 8. 面试时怎么“短讲” atomic？

你可以练习这样一段话（控制在 30 秒～1 分钟）：

> C++11 引入 `std::atomic` 来解决多线程下单个变量读写的数据竞争问题。  
> 普通类型在多线程同时读写是未定义行为，而 `std::atomic<T>` 提供原子性和内存序保障。  
> 简单场景，比如计数器，我用 `std::atomic<long long>` 配合 `fetch_add` 和 `memory_order_relaxed`；  
> 需要建立跨线程“写数据 → 设置标志 → 另一个线程看到标志再读数据”的场景，就用 `store(release)` 和 `load(acquire)` 建立 happens-before。  
> 更复杂的状态更新仍然用 mutex 保护，atomic 只适合单变量、简单不变量的高效同步。

---

# memory_order 
我直接帮你把 `memory_order` 从 0 基础讲清楚，并给一个**“最少够用”用法表**，你面试/写代码先照这个来就行。

---

## 1. 一共有几种 `memory_order`？

C++ 标准里一共 6 种（枚举值）：

- `std::memory_order_relaxed`
- `std::memory_order_consume`（基本废弃，不用）
- `std::memory_order_acquire`
- `std::memory_order_release`
- `std::memory_order_acq_rel`
- `std::memory_order_seq_cst`

**实战建议**：你只要先会这 4 个就够了：

- `relaxed`
- `acquire`
- `release`
- `seq_cst`

`acq_rel` 是 acquire + release 的组合，理解了前两个自然就懂。

---

## 2. 一个核心问题：它到底在控制什么？

`memory_order` 不影响“**这次原子操作本身会不会撕裂**”，那是 atomic 自动保证的。

它控制的是：

> 这次原子操作**前后**的普通读写（对别的变量）在各个线程里能不能**乱序**、能不能**互相看见**。

直觉版：

- `relaxed`：只保证这次原子操作是“整体的”，**但谁先谁后、看不看得见我不管**。
- `acquire/release`：两线程之间建立“**先后顺序**”。
- `seq_cst`：在所有线程看来，所有 seq_cst 原子操作好像按一个**全局的顺序**执行（最强、最保守）。

---

## 3. 最常用的三种模式：怎么用？

### 3.1 `memory_order_relaxed`：只要原子性，不关心顺序

场景：**纯计数、纯统计、你不关心谁先谁后的情况。**

```cpp
std::atomic<long long> counter{0};

void worker() {
    for (int i = 0; i < 1000000; ++i) {
        counter.fetch_add(1, std::memory_order_relaxed);
    }
}
```

- 多线程加出来的总和是对的（不会 data race）。
- 但你不能拿它来推断“哪个线程先干完”、“这之前那之后一定发生了什么”，它不保证。

你可以记：**relaxed = 只要原子，不要顺序**。

---

### 3.2 `release` + `acquire`：典型“写好数据 → 设置就绪标志 → 另一个线程看到标志后读数据”

这是最经典也是最重要的用法。

```cpp
struct Data { int x; int y; };

std::atomic<bool> ready{false};
Data g_data;

void producer() {
    g_data.x = 42;
    g_data.y = 100;
    // 这之前的写（x,y）在 release 之前
    ready.store(true, std::memory_order_release);
}

void consumer() {
    while (!ready.load(std::memory_order_acquire)) {
        // 自旋等待
    }
    // 一旦看到 ready == true，保证能看到 x,y 的正确值
    int a = g_data.x; // 42
    int b = g_data.y; // 100
}
```

保证的是：

- `producer` 里：
  - 所有在 `store(..., release)` **之前**对普通内存的写（`g_data.x/y`）  
- `consumer` 里：
  - 在 `load(..., acquire)` **之后**的普通读（`g_data.x/y`）

**之间建立 happens-before**。  
→ `consumer` 看到 `ready == true` 时，一定能看到 `g_data` 的完整初始化结果。

**记忆法：**

- **写线程**：最后把“旗子”插上 → `store(..., release)`
- **读线程**：看到旗子才往下执行 → `load(..., acquire)`

---

### 3.3 `memory_order_seq_cst`：最简单但最保守

如果你不想细想顺序，保底写法是：

- 所有原子操作都用默认的 `memory_order_seq_cst`（顺序一致性）

```cpp
std::atomic<int> x{0};

void f() {
    x.store(1);          // 等价于 store(1, seq_cst)
    int v = x.load();    // 等价于 load(seq_cst)
}
```

特点：

- 行为最容易理解：所有线程都看到原子操作按某个统一顺序执行。
- 代价：在某些架构/高性能场景下，开销会比 acquire/release 大一点。

**入门/普通业务代码**：完全可以先全用 `seq_cst`，稳定正确最重要。

---

## 4. 其它两个：`acq_rel` 和几乎不用的 `consume`

- `memory_order_acq_rel`：
  - 常用于 `fetch_add`、`compare_exchange` 这种既读又写的操作。
  - 成功时既有 acquire 又有 release 语义（读到别人的写 & 把自己的写发布出去）。

- `memory_order_consume`：
  - 标准里定义得很复杂，大部分编译器实现不完全，现实中**几乎没人用**。
  - 你可以直接当它不存在：用 `acquire` 就好。

---

## 5. 最简单的“用法指南”（可以贴在 day8 笔记里的）

你先记这张小表就够用了：

- **只要原子，不要顺序（计数器、统计）**  
  → `fetch_add(..., memory_order_relaxed)`
- **典型“写数据 + 标志位 + 另一线程看到标志后读数据”**  
  → 写方：`flag.store(true, memory_order_release)`  
  → 读方：`flag.load(memory_order_acquire)`
- **懒得细想顺序，但要正确**  
  → 所有原子操作都用默认 `seq_cst`（不填参数就是它）
- **复杂场景（无锁队列/栈、双向通信）**  
  → 保底先用 `seq_cst`，真要极限性能再考虑 acq_rel/relaxed 优化，一般面试不会让你现场写这么底层的 lock-free 结构。

---

## 6. 你现在可以做到什么程度就算“够用”？

面试/实战上，只要你能：

- 说出 **memory_order 一共有这些枚举，大部分场景只用到 relaxed / acquire / release / seq_cst**；
- 写出一个 **“producer 写数据 + 设置 ready 标志；consumer 用 acquire 等 ready，然后读数据”** 的例子；
- 知道 **atomic + relaxed 可以避免 data race，但不能拿它推断先后顺序**；

基本就已经超过大量普通 C++ 开发者了。

# 死锁

下面把“死锁”这块展开讲清楚，尽量结合 C++ 代码来理解。

---

## 1. 死锁到底是什么？

经典定义：  
多个线程**互相等待对方持有的资源（锁）**，形成闭环，所有等待都无法被满足，程序卡死不动。

在 C++ 里，最典型的是多个 `mutex` 交叉加锁：

```cpp
std::mutex m1, m2;

void t1() {
    std::lock_guard<std::mutex> lk1(m1);
    std::this_thread::sleep_for(10ms);
    std::lock_guard<std::mutex> lk2(m2); // 等 m2
}

void t2() {
    std::lock_guard<std::mutex> lk2(m2);
    std::this_thread::sleep_for(10ms);
    std::lock_guard<std::mutex> lk1(m1); // 等 m1
}
```

- `t1` 持有 `m1` 等 `m2`  
- `t2` 持有 `m2` 等 `m1`  
→ 两边都等不到，死锁。

---

## 2. 死锁的“四个必要条件”（经典 OS 理论）

1. **互斥**：资源一次只能被一个线程占用（锁就是如此）。  
2. **保持并等待**：线程已经持有资源，还在继续申请新的资源。  
3. **不可剥夺**：资源不能被强行抢走，只能线程自己释放。  
4. **循环等待**：存在环形等待链，如 A 等 B，B 等 C，C 等 A。

死锁发生 = 这 4 条同时成立。  
避免思路也就是想办法**破坏其中一条**，实践中最常用的是打破“循环等待”。

---

## 3. 几种常见死锁模式

### 3.1 多把锁加锁顺序不一致

上面例子就是：  
- `t1`：`m1 -> m2`  
- `t2`：`m2 -> m1`  

解决：**全局统一加锁顺序**，例如规定：`&m1 < &m2` 则先锁 m1 再锁 m2：

```cpp
void lock_two(std::mutex& a, std::mutex& b) {
    if (&a < &b) {
        std::lock(a, b);
    } else {
        std::lock(b, a);
    }
}

void t1() {
    std::lock_guard<std::mutex> lk1(m1, std::adopt_lock);
    std::lock_guard<std::mutex> lk2(m2, std::adopt_lock);
}
```

更简单：直接用 `std::scoped_lock`：

```cpp
void t1() {
    std::scoped_lock lk(m1, m2); // 内部用 std::lock，避免死锁
}
```

只要**所有地方都用同样的方式同时拿两把锁，就不会死锁**。

---

### 3.2 锁 + 条件变量：忘记在持锁时等待

错误写法（伪代码）：

```cpp
std::mutex m;
std::condition_variable cv;
bool ready = false;

void consumer() {
    std::lock_guard<std::mutex> lk(m);
    while (!ready) {
        cv.wait(lk); // 错：需要 unique_lock，且 wait 内部会解锁再锁
    }
}
```

正确模式：

```cpp
void consumer() {
    std::unique_lock<std::mutex> lk(m);
    cv.wait(lk, [] { return ready; });
}
```

- `wait` 在睡眠前会**解锁 mutex**，被唤醒后再重新加锁。  
- 如果你用 `lock_guard` 或自己写 `while (cond) {}` 不释放锁，就会把生产者也堵住，形成类似死锁的卡死。

---

### 3.3 锁里再调用可能“反向拿锁”的回调

比如：

```cpp
std::mutex m;
std::function<void()> callback;

void set_callback(std::function<void()> cb) {
    std::lock_guard<std::mutex> lk(m);
    callback = std::move(cb);
}

void notify() {
    std::lock_guard<std::mutex> lk(m);
    if (callback) callback(); // 回调里如果再尝试拿 m，会死锁
}
```

- 回调可能来自别的模块/线程，里面如果也试图 `lock_guard<std::mutex> lk(m);`，就死锁自己。
- 解决：**在锁外调用回调**：

```cpp
void notify() {
    std::function<void()> cb;
    {
        std::lock_guard<std::mutex> lk(m);
        cb = callback; // 拷贝出来
    }
    if (cb) cb(); // 锁外调用
}
```

---

## 4. 怎么系统性避免死锁？

可以总结成几条工程实践：

1. **能只用一把锁就别用两把锁**  
   - 尽量保持锁粒度简单；多把锁就要考虑顺序问题。

2. **必须多锁时，统一顺序**  
   - 约定：`A < B < C`，所有地方都按这个顺序拿锁。  
   - 或者直接用 `std::lock(A, B)` / `std::scoped_lock(A, B)`。

3. **锁内逻辑要短，不要在锁内做 I/O / 阻塞操作**  
   - 网络、磁盘、`sleep`、`join` 之类都不要在持锁时做，容易造成长时间占锁，别人等你 → 更易引入复杂死锁。

4. **避免在持锁状态下调用“未知代码”**（回调、虚函数、外部模块接口）  
   - 很多库专门强调：**回调可能被在持锁状态下调用，请不要在回调里调用回库函数**，就是怕死锁。

5. **条件变量要配合 `unique_lock` 和循环**  
   - 永远使用：
     ```cpp
     std::unique_lock<std::mutex> lk(m);
     cv.wait(lk, [] { return 条件; });
     ```
   - 不要自己 busy-wait，不要 `lock_guard` 配合 `wait`。

---

## 5. 面试时可以怎么讲死锁？

你可以用这样一段话概括（1 分钟左右）：

> 死锁本质上是多个线程互相等待对方持有的锁，满足互斥、保持并等待、不可剥夺、循环等待这四个条件。  
> 在 C++ 里最常见的是多把 mutex 加锁顺序不一致，比如线程 A 拿 m1 再拿 m2，线程 B 拿 m2 再拿 m1。  
> 工程上我会统一加锁顺序，或者使用 `std::lock` / `std::scoped_lock` 一次性拿多把锁；  
> 同时避免在持锁状态下调用未知回调、做阻塞 I/O 或长时间计算。  
> 条件变量配合 `unique_lock` 使用，`wait` 会在睡眠前释放锁，避免自己把生产者堵住造成“伪死锁”。

