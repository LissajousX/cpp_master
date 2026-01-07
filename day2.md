## Day2：RAII 与异常安全

### 理论速览
- RAII：资源获取即初始化，构造获取资源，析构释放，确保异常/早退也能释放。
- 异常安全级别：basic（不变式保持，资源不泄漏），strong（要么成功要么回滚），nothrow（不抛）。
- 析构不抛：析构函数应为 `noexcept`，避免栈展开期间再次抛异常导致 `std::terminate`。
- copy-and-swap：用值传递参数 + swap 提供强异常安全（先构造临时，再交换）。
- 资源包装：文件句柄、锁、动态内存、互斥等都用 RAII 封装；避免裸 new/delete、fopen/fclose 成对出现。

### 最小代码示例
```cpp
#include <cstdio>
#include <stdexcept>
#include <utility>

struct File {
    explicit File(const char* path) : f(std::fopen(path, "w")) {
        if (!f) throw std::runtime_error("open failed");
    }
    ~File() noexcept { if (f) std::fclose(f); }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept : f(other.f) { other.f = nullptr; }
    File& operator=(File other) noexcept { swap(other); return *this; }

    void swap(File& o) noexcept { std::swap(f, o.f); }

    std::FILE* get() const noexcept { return f; }
private:
    std::FILE* f{};
};

int main() {
    File f("/tmp/day2.txt");
    std::fputs("hello", f.get());
    // copy-and-swap 赋值保证强异常安全：先构造临时，后 swap。
    File g("/tmp/day2_bak.txt");
    f = std::move(g); // 资源交换
    return 0;
}
```

### 练习题（含参考作答）
1) 列举 RAII 能解决的典型问题，并给出一个非 RAII 代码的坑点示例。  
   - 典型问题：忘记释放资源（泄漏）、异常早退未解锁/未关闭文件、重复释放。  
   - 非 RAII 坑点示例：`m.lock(); risky(); m.unlock();` 如果 `risky()` 抛异常，未解锁导致死锁；改用 `std::lock_guard`。  
2) 写一个 `unique_lock` 风格的互斥封装，支持移动、不支持拷贝，析构自动解锁。  
   ```cpp
   struct LockGuard {
       explicit LockGuard(std::mutex& m): pm(&m) { pm->lock(); }
       ~LockGuard() noexcept { if (pm) pm->unlock(); }
       LockGuard(const LockGuard&) = delete;
       LockGuard& operator=(const LockGuard&) = delete;
       LockGuard(LockGuard&& o) noexcept : pm(o.pm){ o.pm=nullptr; }
       LockGuard& operator=(LockGuard&& o) noexcept {
           if(this!=&o){ if(pm) pm->unlock(); pm=o.pm; o.pm=nullptr; }
           return *this;
       }
   private: std::mutex* pm{};
   };
   ```  
3) 说明 basic / strong / nothrow 异常安全级别，并给出 vector push_back 的异常安全级别。  
   - basic：不变式保持、不泄漏，但状态可能部分修改。  
   - strong：要么成功要么回滚到原状态。  
   - nothrow：承诺不抛。  
   - `vector::push_back`：若需要扩容且元素移动/拷贝可能抛，多为 basic；元素移动 noexcept 时可达 strong。  
4) 为一个持有原始指针的类添加拷/移 5 函数，保证不泄漏、不重释放，赋值满足 strong 保证。  
   ```cpp
   struct PtrBox {
       explicit PtrBox(int v=0): p(new int(v)) {}
       ~PtrBox(){ delete p; }
       PtrBox(const PtrBox& o): p(o.p? new int(*o.p):nullptr) {}
       PtrBox& operator=(PtrBox o) noexcept { swap(o); return *this; } // copy-and-swap 强保证
       PtrBox(PtrBox&& o) noexcept : p(o.p){ o.p=nullptr; }
       void swap(PtrBox& o) noexcept { std::swap(p,o.p); }
   private: int* p{};
   };
   ```  
5) 把 copy-and-swap 改成显式拷赋+移赋两版，对比异常安全与性能取舍。  
   - 显式拷赋：先分配新资源，成功后释放旧资源，仍可做到 strong，但需额外分支代码。  
   - 显式移赋：直接接管指针并置空源，更高效；若有多资源需小心部分移动失败（通常移动应 noexcept）。  
   - copy-and-swap 简洁、强保证但多一次拷/移和 swap；显式实现可减少一次 swap 但代码更繁琐。  
6) 设计一个函数：内部先分配资源再执行用户回调，要求无泄漏、无 double-free，即便回调抛异常也安全。  
   ```cpp
   void with_file(const char* path, auto&& cb) {
       File f(path);              // RAII 获取
       cb(f);                     // 回调抛异常也会走 f 析构
   } // 离开作用域自动 fclose
   ```

### 自测要点（回答并举例）
- 什么是 RAII，哪些资源适合用 RAII 包装？  
  构造获取资源、析构释放资源；作用域结束（正常/异常）自动清理。适合文件句柄、锁、socket、内存、线程、DB 连接等成对申请/释放的资源。
- 异常安全的 basic / strong / nothrow 定义与示例。  
  basic：不变式保持、不泄漏，状态可能变（如 vector push_back 扩容时拷贝可能抛，容量或元素顺序可能变化）。  
  strong：要么成功要么回滚原状态（copy-and-swap 赋值）。  
  nothrow：承诺不抛（如大多数移动操作期望 noexcept）。
- 析构为何应 noexcept？如果析构可能失败怎么办（记录、吞掉、补偿）。  
  栈展开时析构再抛异常会触发 `std::terminate`；析构应 `noexcept`，失败时记录日志/错误码，或提供显式 `close()/flush()` 让调用者检查，析构里只做尽力清理。
- copy-and-swap 如何提供强异常安全？代价是什么（一次拷/移 + swap）。  
  先构造临时（失败则原对象不变），再与当前对象 swap（通常 noexcept），最后临时析构释放旧资源；代价是额外一次拷/移和一次 swap。
- 资源类的拷/移策略如何选择？何时 delete 拷贝，何时保留移动。  
  独占资源：delete 拷贝，default/自定义移动且 noexcept。可共享资源：用共享语义（如 shared_ptr）。若既要拷又要移，显式声明五/六个特殊成员，避免隐式抑制，并标记移动 noexcept 方便容器选择移动路径。
