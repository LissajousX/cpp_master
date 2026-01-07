## RAII
RAII（Resource Acquisition Is Initialization，资源获取即初始化）要点：

- 核心思路：把资源的获取与释放绑定到对象的生命周期。构造函数里获取，析构函数里释放；无论正常返回还是异常/早退，都靠析构自动清理。
- 适用资源：文件句柄、互斥锁、动态内存、socket、线程、数据库连接等任何需要成对申请/释放的东西。
- 好处：
  - 自动释放，避免泄漏和 double-free。
  - 异常安全：栈展开时析构必定执行，不需要到处写 try/catch 清理。
  - 接口更简洁：持有 RAII 对象即可，不必手动记释放顺序。
- 要点实践：
  - 析构应 `noexcept`，不要抛异常（栈展开时再抛会 `std::terminate`）。
  - 明确拷/移策略：可移动不可拷贝（`=delete` 拷贝，`=default` 移动）常用于独占资源；或显式写五/六个特殊成员。
  - 避免裸 `new/delete`、`fopen/fclose` 等成对调用，统一封装到类型里。
  - copy-and-swap 可提供强异常安全：赋值时先构造临时再交换。
- 小示例（文件句柄封装）：
```cpp
struct File {
    explicit File(const char* p) : f(std::fopen(p, "w")) {
        if (!f) throw std::runtime_error("open failed");
    }
    ~File() noexcept { if (f) std::fclose(f); }

    File(const File&) = delete;
    File& operator=(const File&) = delete;
    File(File&& o) noexcept : f(o.f) { o.f = nullptr; }
    File& operator=(File o) noexcept { swap(o); return *this; }

    void swap(File& o) noexcept { std::swap(f, o.f); }
    std::FILE* get() const noexcept { return f; }
private:
    std::FILE* f{};
};
```
### copy-and-swap 的作用与机制
copy-and-swap 的作用与机制（保证强异常安全）：

- 核心步骤：赋值运算符按值（或右值）接收实参 → 先构造一个临时副本（若构造失败，当前对象不变） → 调用 [swap]交换当前对象与临时的资源 → 临时析构，旧资源被释放。
- 为什么有强异常安全：唯一可能抛异常的是“构造临时”这一步。如果失败，原对象未被修改；如果成功，后续 [swap] 通常是 `noexcept`，因此最终状态要么成功完成、要么保持原状。
- 代价：相比直接拷贝/移动赋值，多了一次构造临时（一次拷贝或移动）和一次 swap，对大对象可能有额外开销。
- 适用场景：需要强异常保证的赋值（如资源类、容器内部实现），且能接受额外一次拷/移的成本。
- 简化示例：
```cpp
struct Resource {
    Resource() = default;
    Resource(const Resource&);            // 可能抛
    Resource(Resource&&) noexcept;        // 移动不抛
    Resource& operator=(Resource other) noexcept { // 传值，other 是临时
        swap(other);                      // swap 通常 noexcept
        return *this;                     // other 析构时释放旧资源
    }
    void swap(Resource& o) noexcept { /* 交换内部指针/句柄 */ }
};
```
- 要点：[swap]需 `noexcept`，移动构造最好 `noexcept`，否则容器/调用方可能无法走移动路径。

### 典型面试点：
  1) 为什么析构要 `noexcept`？  
     - 栈展开时如果析构再抛异常会触发 `std::terminate`，程序直接中止，清理也无法完成。  
     - 资源释放失败（如 fclose 失败）应记录/吞掉或通过显式接口反馈，析构本身不应抛。  
  2) 如何设计 RAII 互斥锁封装？  
     - 构造时加锁，析构时解锁，确保异常/早退也解锁。  
     - 不可拷贝（delete 拷构/拷赋），可移动（default/自定义移动且 noexcept）。  
     - 典型形态：`std::lock_guard<std::mutex>`。  
  3) basic / strong / nothrow 异常安全级别是什么？vector push_back 命中哪个？  
     - basic：操作失败不破坏不变式、不泄漏，状态可能改变。  
     - strong：要么成功要么回到原状态（类似事务）。  
     - nothrow：承诺不抛。  
     - `std::vector::push_back`：若需要扩容且元素移动/拷贝可能抛，则通常提供 basic；如果元素移动是 noexcept，则能提供强保证（迁移元素不抛）。  
  4) 只自定义析构时为什么默认拷贝赋值被删除？  
     - 规则：有用户定义析构 → 编译器不生成隐式拷贝赋值（可能也影响移动赋值的生成）。原因是编译器不再假定默认拷贝赋值对你管理的资源是安全的。  
     - 如果仍需拷贝/移动，必须显式写出或 `=default` 相应函数，并为移动标 `noexcept` 方便容器选择移动路径。