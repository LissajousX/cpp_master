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
