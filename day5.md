## Day5：SFINAE / enable_if（无 concepts）+ 模板重载机制

### 理论速览
- SFINAE（Substitution Failure Is Not An Error，替换失败不是错误）：模板参数替换阶段失败时，该候选被丢弃而不是导致整个编译报错，用于约束模板重载。
- `std::enable_if`：基于布尔条件“开/关”某个函数模板或重载（通常用在返回类型或额外模板参数位置）。
- `std::void_t`：将一组类型/表达式统一映射为 `void`，常用于“如果某个表达式有效，则参与重载”的检测模式。
- 重载决议：普通函数、模板函数、偏特化/重载、SFINAE 参与下，编译器如何选择“最匹配”的候选。
- ADL（Argument-Dependent Lookup，参数相关查找）：函数调用时，会根据实参类型额外在相关命名空间中查找重载，影响模板库中“非成员函数 + using std::swap; swap(a,b);”一类写法。

### 最小代码示例
```cpp
#include <iostream>
#include <type_traits>
#include <utility>

// ===== 1. 使用 enable_if 检测“可加法”的函数模板 =====

// 版本一：若 T+T 表达式合法，则启用此重载

template<class T>
using add_result_t = decltype(std::declval<T>() + std::declval<T>());

// enable_if 放在返回类型位置

template<class T>
std::enable_if_t<std::is_arithmetic_v<add_result_t<T>>, add_result_t<T>>
add_if_addable(T a, T b) {
    std::cout << "add_if_addable(T,T) enabled\n";
    return a + b;
}

// Fallback：如果上面模板因替换失败被丢弃，则匹配这个重载
inline void add_if_addable(...) {
    std::cout << "add_if_addable(...) fallback\n";
}

// ===== 2. 使用 void_t 检测是否存在 operator+ 成员/非成员 =====

// 检测类型 U 是否支持 U+U（不要求 arithmetic，只要表达式合法即可）

template<class, class = void>
struct is_addable : std::false_type {};

template<class U>
struct is_addable<U, std::void_t<decltype(std::declval<U>() + std::declval<U>())>>
    : std::true_type {};

// 带 void_t 约束的版本

template<class T>
std::enable_if_t<is_addable<T>::value, add_result_t<T>>
add_if_addable_v2(T a, T b) {
    std::cout << "add_if_addable_v2(T,T) enabled (void_t)\n";
    return a + b;
}

inline void test_addable() {
    int x = 1, y = 2;
    add_if_addable(x, y);      // 命中模板版本
    add_if_addable_v2(x, y);   // 命中 void_t 版本

    struct S {}; S s{};
    add_if_addable(s, s);      // 回退到 ... 版本
    // add_if_addable_v2(s, s); // SFINAE: 模板被丢弃，无法调用

    std::cout << std::boolalpha
              << "is_addable<int> = " << is_addable<int>::value << "\n"
              << "is_addable<S>   = " << is_addable<S>::value   << "\n";
}

int main() {
    std::cout << "== SFINAE / enable_if / void_t ==\n";
    test_addable();
}
```

> 说明：真正实现时，你可以把上面的代码拆到 `src/day5` 目录下，专门做一份“可加法检测 + traits”示例程序，本文件只是展示核心结构和需要掌握的点。

### 练习题
1) 使用 `std::enable_if` 写两个重载版本的 `print_if_integral`：
   - 当 `T` 是整数类型时生效，打印值；
   - 当 `T` 不是整数类型时，打印“非整型，不打印具体值”。
2) 用 `std::void_t` 写一个 trait `has_size<T>`，在编译期判断 `T` 是否有 `size()` 成员函数。
3) 实现一个 `call_if_addable(a, b)`：
   - 如果 `a + b` 有效，就返回 `a + b`；
   - 否则编译期禁用该重载（调用处必须处理不能调用的情况）。
4) 写两个名字相同的函数 `foo`：
   - 一个放在全局命名空间；
   - 一个放在自定义命名空间 `ns` 内。再定义一个类型 `ns::X`，调用 `foo(x)`，观察 ADL 如何选择重载。

### 自测要点（回答并尽量用自己的话复述）
- 什么是 SFINAE？它在模板重载选择中起到什么作用？常见的使用位置有哪些（返回类型、额外模板参数、默认模板参数）？
- `std::enable_if` 与 `std::void_t` 各自适合什么场景？分别给出一个“检测某表达式是否有效”的用法。
- 重载决议大致顺序：普通函数 vs 模板函数、模板特化/偏特化、SFINAE 过滤之后如何选出“最匹配”的？
- 什么是 ADL？为什么标准库里喜欢写 `using std::swap; swap(a, b);` 而不是直接 `std::swap(a, b);`？
