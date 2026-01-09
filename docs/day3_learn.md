## 对象布局
简要说明对象布局相关要点，并给出小例子：

1) 成员顺序与填充  
- 成员按声明顺序排布，编译器会插入填充字节以满足每个成员的对齐。  
- `sizeof` ≥ 各成员尺寸之和，且是整体对齐的倍数。  
- 示例：`struct { char c; int i; };` 常见布局：`c(1)+填充(3)+i(4)`，总 `sizeof`=8。

2) 继承下的子对象布局  
- 派生类对象包含基类子对象，再加上派生类自己的成员（以及可能的填充）。  
- 单继承下，基类子对象通常在开头；多继承或虚继承会更复杂（有额外指针/偏移）。  
- 空基类优化（EBO）：空基类作为基类时通常不额外占字节（但作为成员会占至少 1 字节）。

3) 标准布局类型的约束（standard layout）  
满足以下条件（简化总结）才能是标准布局：  
- 所有非静态数据成员具有相同的访问控制（都 public 或都 private/protected）。  
- 没有虚函数或虚基类。  
- 所有非静态成员都是标准布局类型。  
- 若有基类，只有一个基类且也是标准布局，且没有与基类成员同名的非静态成员。  
用途：保证与 C 兼容的内存布局，可用于 `memcpy`/二进制序列化/与硬件或网络协议对齐等场景。

4) 对齐（alignment）如何影响布局  
- 每个成员按自身对齐要求放置，整体 `sizeof` 会向最大对齐值对齐。  
- `alignof(T)` 给出类型对齐，`alignas` 可调整对齐（需不降低平台默认要求）。  
- 不满足对齐要求的访问是 UB。

5) 小示例（布局观察）  
```cpp
#include <iostream>
#include <type_traits>

struct Empty {};
struct Base { int b; };
struct Derived : Base { char c; };

struct Packed { char c; int i; double d; };

int main() {
    std::cout << sizeof(Empty) << "\n";      // 1（但 align 可能是 1）
    std::cout << sizeof(Base) << "\n";       // 4
    std::cout << sizeof(Derived) << "\n";    // 8（常见：b:4 + c:1 + 填充3）
    std::cout << sizeof(Packed) << "\n";     // 16（因对齐/填充）
    struct EBO1 : Empty { int x; };
    struct EBO2 { Empty e; int x; };
    std::cout << sizeof(EBO1) << " vs " << sizeof(EBO2) << "\n"; // EBO1 更小
    static_assert(std::is_standard_layout_v<Packed>, "std layout");
}
```

关键记忆：  
- 成员顺序固定，填充为对齐服务。  
- 基类子对象包含在内；虚继承/多继承会增复杂度。  
- 标准布局=无虚、单一标准布局基类、同一访问控制、无同名成员，可用于 C 互操作。  
- 对齐不足访问是 UB，必要时用 `alignas`/`alignof` 检查。

## 几个cast

这里先讲常用 C++ 转换（cast），用途+典型例子+注意事项：

1) `static_cast`
- 用途：编译期已知的安全/可检查转换（数值、指针上下行不跨虚、void* 来回、去掉左值转右值引用不行）。
- 示例：
  ```cpp
  double d = 3.14;
  int i = static_cast<int>(d);          // 截断
  void* pv = &i;
  int* pi = static_cast<int*>(pv);      // void* 恢复
  ```
- 注意：不做运行时类型检查，向下转基类指针若类型不对会 UB。

2) `const_cast`
- 用途：增删 cv 限定（const/volatile）。
- 示例：
  ```cpp
  const int x = 42;
  int* p = const_cast<int*>(&x); // 去 const
  ```
- 注意：对本身是 const 对象写入是 UB，只能用于原本非 const 但经接口加了 const 的场景（如 C 接口）。

3) `reinterpret_cast`
- 用途：位级重解释指针/整数（比如指针与整数互转、不同类型指针互转）。
- 示例：
  ```cpp
  std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(&x);
  int* p = reinterpret_cast<int*>(addr);
  ```
- 注意：强烈不安全，除非满足严格别名/对齐要求，否则 UB；尽量避免。

4) `dynamic_cast`
- 用途：有虚函数的多态体系下安全向下转/横转，运行时检查失败返回 nullptr（指针）或抛 bad_cast（引用）。
- 示例：
  ```cpp
  Base* pb = new Derived;
  if (auto pd = dynamic_cast<Derived*>(pb)) { /* 成功 */ }
  ```
- 注意：仅用于多态类型（有虚函数）；有运行时开销。

5) C 风格/函数风格 `(T)x` / `T(x)`
- 等价于按顺序尝试 const/static/reinterpret 的组合，可能做过度转换，容易踩坑。
- 建议：用上面四种显式 cast，意图更清晰。

6) `std::move`/`std::forward`（虽然不叫 cast，本质是转换）
- `std::move(x)` 等价 `static_cast<T&&>(x)`，无条件把表达式转成 xvalue，提示可移动。
- `std::forward<T>(x)` 根据 T 保留左/右值（用于转发引用场景）。

7) placement new 的“转换”形式
- 形如 `new (buf) T(args...)`，在指定地址构造对象，不分配内存；需显式析构 `obj->~T()`。

常见陷阱与建议：
- 向下转用 `dynamic_cast`（多态）或仔细保证静态类型正确；不要用 `reinterpret_cast` 硬转。
- 去 const 后写入原本 const 对象是 UB。
- `reinterpret_cast` 避免跨不兼容类型的重解释，严格别名/对齐不满足会 UB。
- 首选显式 cast（static/dynamic/const），少用 C 风格。