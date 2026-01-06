## Day1：值类别与移动语义（C++11 为主，C++17 点缀）

### 理论速览
- 值类别：lvalue / xvalue / prvalue；C++17 起 prvalue 不再强制产生临时，帮助 NRVO。
- 引用折叠：`T& & -> T&`，`T& && -> T&`，`T&& & -> T&`，`T&& && -> T&&`；完美转发依赖折叠。
- 拷/移 5/6 函数：拷构、拷赋、析构、移构、移赋（若需要自定义默认构造则为 6）。`=default` / `=delete` 显式意图。
- std::move / std::forward：`std::move` 仅转成 xvalue，不保证移动；`std::forward<T>` 保持实参值类别（只用于模板转发）。
- 返回值优化（RVO/NRVO）：编译器消去临时；C++17 强制性 RVO 覆盖某些场景（返回局部匿名对象）。
- 移动抑制规则：自定义拷构/拷赋/析构会影响编译器生成的移动操作（仅有析构≠删除移动；有拷构或拷赋会阻止隐式移动生成）。

### 最小代码示例
```cpp
#include <iostream>
#include <utility>

struct Buffer {
    Buffer(size_t n = 0) : size(n), data(n ? new int[n] : nullptr) {
        std::cout << "ctor\n";
    }
    ~Buffer() {
        delete[] data;
        std::cout << "dtor\n";
    }
    Buffer(const Buffer& other) : size(other.size), data(other.size ? new int[other.size] : nullptr) {
        std::cout << "copy\n";
    }
    Buffer& operator=(Buffer other) { // copy-and-swap，兼容拷/移
        std::cout << "copy/move assign via swap\n";
        swap(other);
        return *this;
    }
    Buffer(Buffer&& other) noexcept : size(other.size), data(other.data) {
        other.size = 0;
        other.data = nullptr;
        std::cout << "move\n";
    }
    void swap(Buffer& other) noexcept {
        std::swap(size, other.size);
        std::swap(data, other.data);
    }
    size_t size{};
    int* data{};
};

Buffer make_buffer() {
    Buffer b(4);
    return b; // C++17 下允许 NRVO/强制性 RVO
}

int main() {
    Buffer a = make_buffer();        // 观察是否打印 copy/move
    Buffer b(std::move(a));          // 显式移动
    Buffer c; c = b;                 // 拷贝赋值（经 swap）
    return 0;
}
```

### 练习题
1) 简答：分别说明 `std::move` 与 `std::forward` 的用途和使用场景；什么情况下移动不会发生？  
2) 简答：当你自定义了拷贝构造、拷贝赋值、析构时，编译器会不会生成移动构造/移动赋值？为什么？  
3) 代码：写一个持有 `FILE*` 的 RAII 类型，提供拷/移 5 函数，确保资源不泄漏并打印各构造/赋值次数。  
4) 代码：编写一个完美转发的工厂函数 `make_wrapper`，要求在内部调用被包装类型的构造，并保持参数值类别。  
5) 观察：修改上面 Buffer 示例，在 `make_buffer` 中返回 `Buffer{8}` 与返回命名变量 `b`，对比输出，解释 NRVO/强制性 RVO 行为。  
6) 迭代：将 Buffer 的赋值改为显式拷赋与移赋两个函数，对比编译器生成代码与输出顺序，体会 copy-and-swap 的好处/代价。

### 自测要点（回答并举例）
- 左值/右值/xvalue/prvalue 定义与差异？  
- 引用折叠规则背下来；`T&&` 的模板形参在传入左值/右值时各会折叠成什么？  
- RVO/NRVO 在 C++17 的保证范围？什么时候仍可能发生移动/拷贝？  
- `noexcept` 在移动构造中的作用（容器是否选择移动）。  
- copy-and-swap 带来的异常安全性与自适应移动的特点。
