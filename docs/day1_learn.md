## 值类别
- lvalue（locator value，可取地址的“位置”）
  - 特征：有持久存储，可取 `&`，能在赋值左侧。
  - 示例：具名对象 `int x; x`，数组元素 `a[i]`，`*p`，返回 `T&` 的调用。
- xvalue（expiring value，将亡值，右值的一种）
  - 特征：资源可被复用/移动，通常不适合长期持有地址。
  - 示例：`std::move(x)`，返回 `T&&` 的调用，`std::move(obj).member`。
  - 作用：告诉库“可以偷资源”，触发移动构造/赋值。
- prvalue（pure rvalue，纯右值）
  - 特征：纯计算结果/临时值（C++17 前常先物化临时）。
  - 示例：`42`，`1+2`，`Foo()`（值返回），`std::pair{1,2}`，lambda。
  - 用途：初始化、实参、重载决议。

## RVO / NRVO
- 目的：按值返回时，直接在调用方位置构造结果，省掉拷贝/移动。
- RVO（Return Value Optimization，返回值优化）：返回匿名临时如 `return T{...};`，C++17 起强制消除拷/移。
- NRVO（Named RVO，具名返回值优化）：返回局部具名变量如 `T x; return x;`，优化是“允许但不保证”；主流编译器在单一路径下通常会做。
- 关键点：优化与否语义不变，只影响是否额外拷/移；C++17 让匿名返回必然零拷贝，具名返回多数情况下也会优化。

## 画表 + 对照示例（值类别速记）
| 表达式示例 | 预期值类别 | 是否可取址 | 可绑定的引用 |
| --- | --- | --- | --- |
| `int x; x` | lvalue | 是 | `T&`, `T const&` |
| `std::move(x)` | xvalue | 否（不应长期持有地址） | `T&&`, `T const&` |
| `foo()` 返回 `T`（值） | prvalue | 否（C++17 起可直接构造到目标） | `T&&`, `T const&` |
| `foo()` 返回 `T&` | lvalue | 是 | `T&`, `T const&` |
| `foo()` 返回 `T&&` | xvalue | 否（不稳定） | `T&&`, `T const&` |
| 字面量 `42` | prvalue | 否 | `T&&`, `T const&` |
| 成员访问 `obj.m` | lvalue | 是 | `T&`, `T const&` |
| 成员访问 `std::move(obj).m` | xvalue | 通常不持有地址 | `T&&`, `T const&` |


## NRVO 可能失效的例子（编译器可能不做，进而有移动/拷贝）：
1) 多分支返回不同具名变量：
```cpp
T f(bool cond) {
    T a, b;
    if (cond) return a;
    else return b; // 两个不同具名对象
}
```
2) 返回的不是局部对象：
```cpp
T g(const T& param) {
    static T s;
    if (param.foo()) return s;  // 返回静态/参数，不是局部新建
    return param;               // 参数对象
}
```
3) 复杂控制流让唯一构造点不明确（有时编译器仍可处理，但不保证）：
```cpp
T h(int n) {
    T x;
    while (n-- > 0) {
        if (n == 1) return x; // 早退路径
    }
    return x;
}
```
4) 宏/条件运算符等导致返回表达式不直接是那个具名局部（取决于编译器）：
```cpp
T i(bool cond) {
    T x, y;
    return cond ? x : y; // 三目含两个具名
}
```
5) 返回局部的子对象/成员引用（本身就不满足值返回 NRVO 场景）：
```cpp
T j() {
    Wrapper w;
    return w.member; // 返回子对象的值，可能需要拷/移
}
```
## 引用折叠
引用折叠规则（折叠后的结果只可能是左值引用 T& 或右值引用 T&&）：
- `T&  &  -> T&`
- `T&  && -> T&`
- `T&& &  -> T&`
- `T&& && -> T&&`

记忆方式：只要有一个 `&`（左值引用）参与，结果就是 `&`；只有 “&& 和 &&” 才保留为 `&&`。

完美转发依赖折叠的原因：
- 模板参数形如 `T&&`（转发引用，曾叫万能引用）：传左值时，推导出 `T` 为 `X&`，代入后是 `X& &&` 折叠成 `X&`；传右值时，`T` 为 `X`，代入后 `X&& &&` 折叠成 `X&&`。
- `std::forward<T>(arg)` 会用折叠后的引用类型把值类别“还原”回去。

最小示例（可在 CE/本地实验，看重载命中）：
```cpp
#include <iostream>
#include <utility>

void f(int&)  { std::cout << "lvalue\n"; }
void f(int&&) { std::cout << "rvalue\n"; }

template<class T>
void wrapper(T&& x) { // x 是转发引用
    f(std::forward<T>(x)); // 保留原值类别
}

int main() {
    int a = 0;
    wrapper(a);          // 推导 T=int&，折叠成 int&，命中 lvalue
    wrapper(1);          // 推导 T=int，折叠成 int&&，命中 rvalue
}
```

总结：
- 折叠规则就是“有 & 则 &，双 && 留 &&”。
- 转发引用 + `std::forward` 利用折叠，让模板保持调用者的值类别，避免把左值错误当右值或反之。

## 完美转发
完美转发核心：用“转发引用”（形如模板参数 `T&&`，函数模板/auto 推导场景）+ `std::forward<T>`，把实参的值类别/const 性原封不动地传下去，避免本应移动的被当成拷贝，本应保持左值的被当成右值。

关键点
1) 转发引用与折叠规则  
- 只有“模板参数推导得到的 `T&&`”才是转发引用（非推导场景的 `T&&` 只是右值引用）。
- 折叠：`T&&` 传左值 → `T` 推导为 `X&`，代入得 `X& &&` 折叠成 `X&`；传右值 → `T=X`，得 `X&&`。

2) `std::forward<T>(arg)` 作用  
- 根据推导的 `T` 恢复原值类别：`T` 若为 `X&`，forward 返回 `X&`；若为 `X`，返回 `X&&`。  
- `std::move` 则总把表达式转成 xvalue，不区分原本是否左值。

3) 典型用法（转发到构造/工厂/回调）  
```cpp
#include <utility>
#include <vector>

template<class T, class... Args>
T make_forward(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

template<class F, class... Args>
auto call(F&& f, Args&&... args) -> decltype(auto) {
    return std::forward<F>(f)(std::forward<Args>(args)...);
}
```

4) 常见陷阱  
- 花括号初始化列表 `{}` 不是对象，不能直接用 `forward` 转发；需要显式构造，如 `make_forward<std::vector<int>>({1,2})` 不行，改成 `make_forward<std::vector<int>>(std::initializer_list<int>{1,2})` 或直接 `std::vector<int>{1,2}`。  
- 区分 `auto&&`（转发引用）与 `const auto&`（会把右值绑定成 const 左值引用，失去移动）。  
- 只在确实需要“保留值类别”时用 `forward`，不要在普通非模板参数上滥用。  
- 如果目标重载对 noexcept 有区分（如容器内部优化），保证移动构造/赋值是 noexcept 才更可能被选中。

<details>
<summary>常见陷阱详解</summary>
下面逐条展开并给出小例子，便于面试回答：

1) `{}` 初始化列表不能直接 forward  
- 原因：`{1,2}` 不是对象，模板推导不会把它当成 `std::initializer_list` 自动 perfect forward。  
- 现象：`make_forward<std::vector<int>>({1,2})` 可能编译失败或匹配不到期望构造。  
- 对策示例：
```cpp
auto v1 = make_forward<std::vector<int>>(std::initializer_list<int>{1,2});
auto v2 = make_forward<std::vector<int>>(std::vector<int>{1,2}); // 直接传对象
```

2) `auto&&` vs `const auto&`  
- `auto&&`（在推导场景）是转发引用：左值传入→左值，右值传入→右值，保留移动机会。  
- `const auto&` 把右值也绑成 const 左值引用，不再移动，但更稳。  
- 示例对比：
```cpp
auto&& rr = std::string("hi");   // rr 是右值引用，可移动
const auto& cr = std::string("hi"); // cr 是 const 左值引用，不会移动
```
选择策略：需要保留值类别/移动 → `auto&&`；只读且不关心移动 → `const auto&` 简单安全。

3) 只在需要保留值类别时用 `forward`  
- `forward` 仅在转发引用（推导得到的 `T&&`）里有意义，非模板形参或明确要右值的场景，用正常传参或 `std::move`。  
- 反例：对一个普通左值参数用 `forward` 试图“强制右值”是错误的（会编译失败或语义不对）。  
- 示例：
```cpp
template<class T>
void wrap(T&& x) { use(std::forward<T>(x)); } // 对：转发引用
void foo(std::string s) { use(std::move(s)); } // 这里用 move，forward 没意义
```

4) noexcept 影响容器重载选择  
- 容器搬运元素时，若移动构造/赋值是 `noexcept`，更倾向走移动；否则可能回退到拷贝以保证异常安全。  
- 示例：
```cpp
struct S {
    S()=default;
    S(const S&){ std::cout<<"copy\n"; }
    S(S&&) noexcept { std::cout<<"move\n"; }
};
std::vector<S> v;
v.emplace_back();
v.push_back(S{}); // 如果移动 noexcept，扩容时会打印 move；否则可能打印 copy
```
- 对策：能保证安全时给移动构造/赋值加 `noexcept`，并保持成员也 noexcept。

面试回答节奏可用模板：先定义/现象 → 原因 → 对策/示例。
</details>

5) 快速检测思路  
- 写 `void f(int&); void f(int&&);`，在模板里转发，观察命中哪个重载，确认值类别保留正确。  
- 配合 `std::is_lvalue_reference_v<decltype((expr))>` 检查推导结果。

6) 什么时候用  
- 泛型工厂/包装器（构造对象、存储到容器、调用回调）需要把调用者语义原样透传时，用转发引用 + forward。  
- 不需要保留值类别时，直接按需求选择拷贝/移动或值传递即可，无需 forward。

<details>
<summary>用最“傻瓜版”语言讲完美转发</summary>

1) 背景问题  
- 我有一个“通用门” `T&& arg`，希望：别人给我左值，我就当左值传下去；给我右值，我就当右值传下去。这样既不误把左值当右值（会偷资源），也不误把右值当左值（少了移动优化）。

2) 哪种 `T&&` 是这种“魔法门”？  
- 只有模板里、靠编译器推导出来的 `T&&` 才有魔法（叫“转发引用”）。  
- 如果类型写死了，比如 `void foo(int&&)`, 这就只是“右值专用口”，左值进不来。

3) 这扇门怎么变形？（引用折叠规律一句话）  
- 只要混到一个 `&`，结果就是 `&`；只有 “&& 和 &&” 才留成 `&&`。  
  - 传左值：推导出 `T=int&`，代入 `T&&` 变 `int& &&`，折叠成 `int&`（左值）。  
  - 传右值：推导出 `T=int`，代入变 `int&&`（右值）。

4) 为什么要 `std::forward`？  
- `forward<T>(arg)` 会按推导出的 `T` 把值类别“还原”给下游：  
  - 如果 `T=int&`，就还原成左值引用；  
  - 如果 `T=int`，就还原成右值引用。  
- 而 `std::move` 是“一刀切转右值”，不管原来是不是左值。

5) 最小例子（看输出感受魔法）  
```cpp
#include <iostream>
#include <utility>

void f(int&)  { std::cout << "lvalue\n"; }
void f(int&&) { std::cout << "rvalue\n"; }

template<class T>
void call(T&& x) {          // 魔法门（转发引用）
    f(std::forward<T>(x));   // 保留原值类别
}

int main() {
    int a = 0;
    call(a);   // a 是左值，输出 lvalue
    call(1);   // 字面量是右值，输出 rvalue
}
```
如果把 `call` 改成 `void call(int&& x)`，则 `call(a)` 会直接编译报错，因为那就不是魔法门了。

6) 什么时候用？  
- 写“泛型包装/工厂/转发”的函数，需要原样透传别人的参数时。  
- 不需要保留值类别时，不必用 forward；按需拷贝或移动即可。

一句话总括：想要自动识别左/右值并原样转出去，就用“模板推导得到的 `T&&` + std::forward<T>(arg)`；否则 `T&&` 只是右值专用口。

用最简单的话说：

- “完美转发”不是一个关键字，也不是单独的函数，而是一种模式/用法：在模板里用“转发引用（`T&&`，且 `T` 由推导得到）+ `std::forward<T>`”，把调用者传入参数的“原汁原味”（值类别、const 性）原样交给下游。
- 它的意义：调用者给左值，你就当左值传下去；给右值，就当右值传下去。这样不会错失移动优化，也不会误偷资源。
- 它依赖引用折叠规则和转发引用：只有模板推导出的 `T&&` 才是转发引用；`std::forward<T>(x)` 根据 `T` 把 x 还原成原来的左/右值。
- 常见用法：泛型工厂/包装器/回调转发。
  - 例：`make_forward<T>(Args&&... args) { return T(std::forward<Args>(args)...); }`
  - 例：`call(F&& f, Args&&... args) { return std::forward<F>(f)(std::forward<Args>(args)...); }`
- 不是完美转发的：写死类型的 `int&&`、非模板参数，或不需要保留值类别的场景。
</details>

## std::move && std::forward
解释要点：

- `std::move(expr)`：
  - 作用：把表达式无条件转成 xvalue（右值类别），提示“可以移走资源”。
  - 不保证一定走移动：是否调用移动构造/赋值取决于目标类型是否有可用且（常见场景下）noexcept 的移动重载，否则可能回退到拷贝。
  - 适用：你确定要把一个左值当右值交出去（通常是准备“失效”它）。

- `std::forward<T>(expr)`：
  - 作用：根据模板参数 T 的推导结果恢复原值类别：
    - 如果 T 被推导为 `U&`，`forward<T>` 返回左值引用。
    - 如果 T 被推导为 `U`，`forward<T>` 返回右值引用。
  - 只在转发引用（推导得到的 `T&&`）场景使用，用来“原汁原味”传递调用者的参数。
  - 适用：完美转发——编写泛型包装/工厂/回调时保留调用者的左/右值语义。

和完美转发的关系：
- 完美转发 = “推导得到的 `T&&`” + `std::forward<T>(arg)`。
- `std::move` 不关心原值类别，强制右值；`std::forward` 按推导结果决定是否右值，从而避免把左值误当右值或把右值降格为左值。

一句话区分：
- 知道自己想强制右值用 `move`。
- 想保留调用者原值类别、写泛型包装时用 `forward<T>` 配合转发引用。


## Rule of Five/Six
拷/移 5/6 函数（又叫 Rule of Five/Six）要点：

包含哪些：
1) 析构函数：`~T()`
2) 拷贝构造：`T(const T&)`
3) 拷贝赋值：`T& operator=(const T&)`
4) 移动构造：`T(T&&)`
5) 移动赋值：`T& operator=(T&&)`
（若需要自定义默认构造，则加上默认构造，共 6 个）

生成/抑制规则（常考）：
- 你一旦自定义了拷贝构造或拷贝赋值，编译器就不再隐式生成移动构造/移动赋值，需要手写 `=default` 或自己实现，反之也成立。
- 只有自定义析构，不会删除移动，但会删除隐式生成的拷贝赋值；同时移动可能被抑制（因拷贝未定义）。实践上，想要移动就显式写出移动或 `=default`。
    >只自定义了析构时，编译器的行为：
    > - 不会删除移动构造/移动赋值（它们依然有机会生成），但
    > - 会删除隐式生成的拷贝赋值（这是规则：有用户析构 → 隐式拷贝赋值被删除，拷贝构造仍可生成）。
    > - 因为拷贝赋值被删除，移动赋值的生成也可能被抑制（有些情况下编译器需要拷贝赋值可用才生成移动赋值）。
- 无用户自定义的拷/移/析构时，编译器可自动生成拷/移。

`=default` / `=delete` 用法：
- `=default;`：显式要求编译器生成默认实现（可放在类内或类外）。
- `=delete;`：显式禁用某个函数（常用来禁止拷贝或禁止从某类型转换）。

推荐模式（资源管理类）：
- 想要“可移动不可拷贝”：`File(const File&)=delete; File& operator=(const File&)=delete; File(File&&)=default; File& operator=(File&&)=default;`
- 想要全功能：显式写出 5/6 个函数，移动构造/赋值最好标 `noexcept` 以利容器优化。

示例（可移动不可拷）：
```cpp
struct Handle {
    Handle() = default;
    ~Handle() = default;

    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;

    Handle(Handle&&) noexcept = default;
    Handle& operator=(Handle&&) noexcept = default;
};
```

示例（全功能，copy-and-swap 赋值）：
```cpp
struct Buffer {
    Buffer() = default;
    ~Buffer() = default;

    Buffer(const Buffer& other) : data(other.data) {} // 简化示例
    Buffer(Buffer&& other) noexcept : data(std::move(other.data)) {}

    Buffer& operator=(Buffer other) noexcept { // 兼容拷/移
        swap(other);
        return *this;
    }
    void swap(Buffer& o) noexcept { data.swap(o.data); }

private:
    std::vector<int> data;
};
```

面试回答模板：
- 先列 5/6 个函数名称。
- 说明自定义拷贝会抑制隐式移动，需要显式写出移动或 `=default`。
- 说明 `=default`/`=delete` 的意图表达，移动最好 `noexcept`。

## 面试题自测（Day1 主题）
1) 区分并举例：lvalue / xvalue / prvalue。哪些引用可以绑定它们？  
2) 解释引用折叠规则，并说明为何“推导得到的 `T&&`”可以同时接收左值/右值。  
3) `std::move` 与 `std::forward` 的区别与使用场景；何时移动不会发生？  
4) 说出 RVO 与 NRVO 的差别、C++17 的强制性 copy elision 范围，以及 NRVO 可能失效的例子。  
5) 迭代器/引用失效：vector 在 push_back/emplace_back/erase/扩容时对引用迭代器的影响。  
6) Rule of Five/Six：列出 5/6 个特殊成员函数，并说明自定义拷贝/移动/析构对隐式生成的影响。  
7) 给出一个“可移动不可拷贝”的资源类写法，并解释为何移动要标 `noexcept`。  
8) 给出一个完美转发工厂/调用包装的最小代码，说明 `{}` 初始化列表为何不能直接 forward。  
9) 何时应返回值 vs 返回引用？结合 RVO/NRVO、安全性与封装性谈取舍。  
10) 为什么“只自定义析构”会删除隐式拷贝赋值？如果仍想要移动/拷贝，该如何写？