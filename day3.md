## Day3：对象模型与内存管理

### 理论速览
- 对象布局：成员顺序、填充字节、继承下的子对象布局；标准布局类型的约束。
- 对齐（alignment）：`alignof`，对齐要求与填充；`std::max_align_t`；`alignas` 调整对齐。
- 空基类优化（EBO）：空基类不占额外空间（大多数实现），用于压缩存储（如 `std::function` 内部、压缩 pair）。
- placement new：在预分配内存上构造对象；需显式调用析构释放。
- 常见 UB：未初始化读取、越界访问、释放后使用、严格别名规则破坏、未对齐访问、二次释放、释放非动态分配内存等。

### 最小代码示例
```cpp
#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <type_traits>

struct Empty {};
struct Base { int b; };
struct Derived : Base { char c; };

struct Packed {
    char c;
    int  i;
    double d;
};

// 简易对象池：固定容量，用 placement new 构造/显式析构
class Pool {
public:
    explicit Pool(std::size_t n) : cap(n), used(0) {
        storage = ::operator new[](cap * sizeof(int));
    }
    ~Pool() {
        // 显式析构已构造的对象
        for (std::size_t i = 0; i < used; ++i) {
            int* p = reinterpret_cast<int*>(static_cast<char*>(storage) + i * sizeof(int));
            p->~int();
        }
        ::operator delete[](storage);
    }
    int* create(int v) {
        assert(used < cap);
        void* slot = static_cast<char*>(storage) + used * sizeof(int);
        ++used;
        return new (slot) int(v); // placement new
    }
private:
    void* storage{};
    std::size_t cap{};
    std::size_t used{};
};

int main() {
    std::cout << "== 布局与对齐 ==\n";
    std::cout << "sizeof(Empty)=" << sizeof(Empty) << " align=" << alignof(Empty) << "\n";
    std::cout << "sizeof(Base)=" << sizeof(Base) << " align=" << alignof(Base) << "\n";
    std::cout << "sizeof(Derived)=" << sizeof(Derived) << "\n";
    std::cout << "sizeof(Packed)=" << sizeof(Packed) << " (可能包含填充)\n";

    static_assert(std::is_standard_layout_v<Packed>, "Packed is standard layout");

    std::cout << "\n== 空基类优化 (EBO) 示例 ==\n";
    struct EBO1 : Empty { int x; };
    struct EBO2 { Empty e; int x; };
    std::cout << "sizeof(EBO1)=" << sizeof(EBO1) << " vs sizeof(EBO2)=" << sizeof(EBO2) << "\n";

    std::cout << "\n== placement new + 显式析构 ==\n";
    Pool pool(3);
    int* a = pool.create(10);
    int* b = pool.create(20);
    std::cout << "a=" << *a << " b=" << *b << "\n";

    std::cout << "\n== 未对齐访问示意（不要这么做） ==\n";
    alignas(1) struct Misalign { char c; double d; }; // 仅示意，可能 UB
    std::cout << "alignof(Misalign)=" << alignof(Misalign) << "\n";
}
```

### 练习题
1) 写一个压缩存储的 pair，当第二个类型可能为空时利用 EBO 减少大小，并对比 `sizeof`。  
2) 用 placement new 构造一个非平凡类型数组，并显式调用析构；用 `static_assert` 检查对齐满足要求。  
3) 列出 5 个 UB 示例，并用 ASan/UBSan 观察各自的报错。  
4) 设计一个标准布局类型并用 `static_assert(std::is_standard_layout_v<T>)` 验证，解释其约束。  
5) 演示未对齐访问的风险：构造一个对齐小于类型要求的缓冲区，说明为何这是 UB（仅解释，不要在生产代码中执行）。  
6) 对比有/无 EBO 的两种类的 `sizeof`，解释差异。  

### 自测要点（回答并举例）
- 什么是对象布局、填充字节，对齐如何影响 `sizeof`？
- 什么是标准布局类型，为什么重要（与 C 兼容、可做内存映射、序列化假设等）？
- EBO 的作用与限制（多重继承同一空基类可能需 unique base 以启用 EBO）。
- placement new 的使用规则：在预留空间构造，需显式析构，不要与普通 delete 混用。
- 列举常见 UB：未初始化读取、越界、释放后使用、未对齐访问、双重释放、违反严格别名等，并说明如何借助 sanitizers 发现。
