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