## Day5：SFINAE / enable_if（无 concepts）+ OOP 基础复盘

### 理论速览
- SFINAE（Substitution Failure Is Not An Error，替换失败不是错误）：模板参数替换阶段失败时，该候选被丢弃而不是导致整个编译报错，用于约束模板重载。
- `std::enable_if`：基于布尔条件“开/关”某个函数模板或重载（通常用在返回类型或额外模板参数位置）。
- `std::void_t`：将一组类型/表达式统一映射为 `void`，常用于“如果某个表达式有效，则参与重载”的检测模式。
- 重载决议：普通函数、模板函数、偏特化/重载、SFINAE 参与下，编译器如何选择“最匹配”的候选。
- ADL（Argument-Dependent Lookup，参数相关查找）：函数调用时，会根据实参类型额外在相关命名空间中查找重载，影响模板库中“非成员函数 + using std::swap; swap(a,b);”一类写法。
- OOP 三大特性：封装、继承、多态。
- 虚函数与动态绑定：`virtual`、虚表、通过基类指针/引用调用重写函数的运行时分派。
- `override` / `final`：`override` 保证真正在重写虚函数；`final` 禁止进一步重写或继承，减少误用。
- 菱形继承与虚继承：多条继承路径导致的重复基类问题；`virtual` 继承合并成一个共享基类子对象，带来指针调整和虚基表的开销。

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

// ===== 3. OOP：菱形继承 + 虚继承示例 =====

struct Base {
    int value = 0;
    virtual void who() const {
        std::cout << "Base::who, value=" << value << "\n";
    }
    virtual ~Base() = default;
};

struct Left : virtual Base {
    Left() { value = 1; }
    void who() const override {
        std::cout << "Left::who, value=" << value << "\n";
    }
};

struct Right : virtual Base {
    Right() { value = 2; }
    void who() const override {
        std::cout << "Right::who, value=" << value << "\n";
    }
};

struct Diamond final : Left, Right {
    Diamond() {
        // 由于虚继承，只有一个 Base 子对象，统一从这里设置
        value = 42;
    }
    void who() const override {
        std::cout << "Diamond::who, value=" << value << "\n";
    }
};

inline void test_oop() {
    std::cout << "sizeof(Base)    = " << sizeof(Base)    << "\n";
    std::cout << "sizeof(Left)    = " << sizeof(Left)    << "\n";
    std::cout << "sizeof(Right)   = " << sizeof(Right)   << "\n";
    std::cout << "sizeof(Diamond) = " << sizeof(Diamond) << "\n";

    Diamond d;
    Base* pb = &d;   // 通过虚继承，Base 子对象唯一
    pb->who();       // 动态绑定，调用 Diamond::who

    Left* pl = &d;
    Right* pr = &d;
    pl->who();       // 仍然最终调用 Diamond::who
    pr->who();
}

int main() {
    std::cout << "== SFINAE / enable_if / void_t ==\n";
    test_addable();

    std::cout << "\n== OOP 虚继承菱形 ==\n";
    test_oop();
}
```

> 说明：真正实现时，你可以把上面的代码拆到 `src/day5` 目录下，分成两个文件（SFINAE 示例 + OOP 示例），本文件只是展示核心结构和需要掌握的点。

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
5) 设计一个简单的 OOP 继承层次：`Shape`（基类）+ `Circle` / `Rectangle`，使用虚函数实现 `area()` 多态，体会封装 + 继承 + 多态的组合。
6) 写一个有菱形继承但**不**使用虚继承的例子，观察两个基类子对象如何导致二义性（例如两个 Base 的成员同名），然后改成虚继承解决问题，并对比 `sizeof` 和虚函数调用行为。

### 自测要点（回答并尽量用自己的话复述）
- 什么是 SFINAE？它在模板重载选择中起到什么作用？常见的使用位置有哪些（返回类型、额外模板参数、默认模板参数）？
- `std::enable_if` 与 `std::void_t` 各自适合什么场景？分别给出一个“检测某表达式是否有效”的用法。
- 重载决议大致顺序：普通函数 vs 模板函数、模板特化/偏特化、SFINAE 过滤之后如何选出“最匹配”的？
- 什么是 ADL？为什么标准库里喜欢写 `using std::swap; swap(a, b);` 而不是直接 `std::swap(a, b);`？
- OOP 三大特性分别对应什么实际代码手段？（比如封装→访问控制+数据隐藏，继承→代码复用+接口扩展，多态→虚函数+动态绑定。）
- `virtual`、`override`、`final` 各自的语义是什么？在哪些场景下**强烈推荐**加上 `override`？
- 菱形继承在没有虚继承时会带来什么问题？虚继承是如何解决“重复基类”的？代价体现在哪些方面（对象布局更复杂、指针调整、额外的间接层次）？
