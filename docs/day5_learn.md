# 编译过程中对于模板函数的处理以及SFINAE规则的应用

## 1. 调用一个普通函数时，编译器在干嘛？

先不管模板，看看最简单的情况：

```cpp
void f(int);
void f(double);

int main() {
    f(10);   // 调用这里
}
```

编译器大致流程（非常简化）：

1. 看到调用点 `f(10)`。
2. 在当前作用域里**找到所有叫 `f` 的函数**：
   - `void f(int)`
   - `void f(double)`
3. 用“重载决议规则”选一个“最匹配”的：
   - `10` 是 `int`，显然 `void f(int)` 最合适。

到这里没有模板，一切都很直观。

---

## 2. 把函数换成模板后，多了哪一步？

现在改成模板：

```cpp
template<class T>
void f(T);

void f(double);  // 非模板版本

int main() {
    f(10);
}
```

流程变成：

1. 看到 `f(10)`。
2. 找到所有叫 `f` 的东西：
   - 函数模板 `template<class T> void f(T);`
   - 普通函数 `void f(double);`
3. **对模板做“模板参数推导 + 代入”**：
   - 看到 `f(10)`，推导 `T = int`；
   - 得到一个“具体版本”：`void f<int>(int)`，它是一个**候选函数**。
4. 现在候选集是：
   - `void f<int>(int)`（模板实例化出的版本）
   - `void f(double)`（普通函数）
5. 再做一次重载决议，选择“最匹配”的：
   - 对 `10` 来说，`f<int>(int)` 比 `f(double)` 更合适，于是选它。

对你来说，关键是多了第 3 步：

> “对模板做**模板参数推导**，然后**代入**，生成一个具体的候选函数。”

---

## 3. 再加点限制：模板声明里出现“可能不合法”的东西

我们让模板函数长得复杂一点：

```cpp
template<class T>
auto foo(T t) -> typename T::value_type;  // 声明依赖 T::value_type

void foo(...); // 兜底版
```

调用：

```cpp
std::vector<int> v;
int x = 0;

foo(v);
foo(x);
```

对每个调用，流程是这样的：

### 调用 `foo(v)` 时

1. 找到候选：
   - 模板 `foo(T)`；
   - 兜底 `foo(...)`。
2. 对模板 `foo(T)` 做“推导 + 代入”：
   - 从调用 `foo(v)` 推导出 `T = std::vector<int>`；
   - 把 `T` 代入返回类型 `typename T::value_type` → 变成 `typename std::vector<int>::value_type`，这是合法的（`vector` 确实有 `value_type`）。
3. 所以模板版本**代入成功**，得到具体函数：
   - `auto foo(std::vector<int> t) -> int;`
4. 候选集现在是：
   - 模板实例化版本 `foo(std::vector<int>)`
   - 兜底 `foo(...)`
5. 重载决议：
   - 对 `v` 这个实参来说，模板版本更匹配 → 选它。

### 调用 `foo(x)` 时

1. 候选仍然是那两个。
2. 对模板 `foo(T)` 做“推导 + 代入”：
   - 从调用 `foo(x)` 推导 `T = int`；
   - 把 `T` 代入返回类型 `typename T::value_type` → 变成 `typename int::value_type`。
3. **这里就炸了**：  
   - `int` 没有 `value_type` 成员；
   - 这条模板在“代入签名”这一步就不合法了。

这时就轮到 SFINAE 出场了：

> 语言规定：  
> 当模板在“**替换模板参数到函数声明中**”这一步失败时，这是一个“Substitution Failure”（替换失败），**不算整个程序的编译错误**，而是说：  
> “这条模板候选作废，丢掉。”

所以：

- 对 `foo(x)`，模板候选被丢弃；
- 候选集只剩下 `foo(...)`；
- 重载决议只能选 `foo(...)`。

这就是你看到的“代入失败 → 回退到默认实现”的真实含义：  
**不是“先成功再回退”，而是“这条模板根本没资格参与后面的选择，只剩默认实现了”。**

---

## 4. SFINAE 规则插在流程的哪个位置？

把刚才整个流程写成一个“时间线”：

1. 找函数名 → 收集所有叫 `foo` 的候选（包括模板和非模板）。
2. 对每个模板候选：
   - 做模板参数推导（比如 `T = int`）；
   - 在**函数声明**里，把 `T` 全部代入；
   - 如果在这一步因为类型/表达式不合法而失败：
     - **SFINAE：把这条候选从列表里剔除**；
     - 但是**不报整个程序错误**。
3. 剩下的候选（模板也好、非模板也好）参与正常的重载决议，选一个最合适的。

你要理解的就是第 2 步里那条规则，就是 SFINAE。

---

## 5. 回答你两个具体困惑

> 1）SFINAE 是个什么东西？编译选项还是代码风格？

- **答案**：既不是编译选项，也不是风格。
- 它是 C++ 标准里描述**“模板代入失败时编译器怎么做”**的一个**语义规则**。

> 2）我不能理解代入失败回退到默认的实现，不一样是代入成功么？

- 不一样。
- “成功”意味着：  
  - 这条模板**留下来**，进入后续重载决议的比较；
- “失败 + SFINAE”意味着：  
  - 这条模板**直接从候选中被删掉**，后续过程里**好像根本没这条函数**；
  - 剩下的候选里，自然可能只剩那个“默认实现”。

你可以把它想象成“应聘过滤”：

- 写了两份 JD（岗位说明）：一个是“高级 C++ 模板工程师 JD”，一个是“普通开发 JD”；
- 全部候选人都来投简历，这是“函数调用”；
- 编译器做的事：
  - 先看每个候选人能不能满足“高级 JD”里的硬性条件（能否 `T::value_type`、能否 `T+T` 等）；
  - 不满足的人**不算“系统异常”，只是被从这个 JD 的候选名单里划掉**，但他们仍然可以去匹配“普通开发 JD”；
- 最后选人时，只在“还活着的 JD + 候选人组合”里做选择。

# enable_if是什么，怎么用的

## 1. 标准库里的 `std::enable_if` 是什么？

在 `<type_traits>` 里，大致长这样（标准允许实现不一样，但语义等价）：

```cpp
namespace std {

template<bool B, class T = void>
struct enable_if {};                 // 主模板：没有成员 type

template<class T>
struct enable_if<true, T> {          // 只有 B==true 才有 type
    using type = T;
};

template<bool B, class T = void>
using enable_if_t = typename enable_if<B, T>::type;  // 这是常用的别名

}
```

也就是说，“真正的 `enable_if`”和我刚才写的那版**本质是一样的**，只是名字在 `std::` 里，而且你不用自己实现，直接：

```cpp
#include <type_traits>
```

就能用了。

**记忆一句话**：

> `std::enable_if_t<条件, 类型>`  
> - 条件为 `true` → 它是一个合法的 `类型`；  
> - 条件为 `false` → 在这一行模板参数替换失败（SFINAE），用到它的那个模板重载会被丢弃。

---

## 2. 真正常用的几种写法（你只需要会这几类）

### 2.1 放在返回类型：开/关一个函数模板

```cpp
#include <type_traits>
#include <iostream>

// 只有整型才启用
template<class T>
std::enable_if_t<std::is_integral_v<T>, void>
print(T x) {
    std::cout << "integral: " << x << "\n";
}

// 兜底版本
void print(...) {
    std::cout << "fallback\n";
}

int main() {
    print(42);     // 命中模板：integral: 42
    print(3.14);   // 模板代入失败 → SFINAE 丢弃 → 命中 fallback
}
```

这里：

- 对 `print(42)`：
  - `T=int` → `std::is_integral_v<int> == true`  
  - 返回类型是 `std::enable_if_t<true, void>` → 就是 `void`，代入成功。
- 对 `print(3.14)`：
  - `T=double` → 条件为 `false`  
  - 返回类型变成 `std::enable_if_t<false, void>`，这行代入失败 → **模板候选被丢弃**。

**这是最常见的一类用法。**

---

### 2.2 放在额外模板参数：不动返回类型

有时你想让返回类型看起来干净点，就这样写：

```cpp
template<class T,
         std::enable_if_t<std::is_integral_v<T>, int> = 0>
void print(T x) {
    std::cout << "integral: " << x << "\n";
}
```

含义和上一个例子一样，只是把 `enable_if_t<...>` 挪到了“**额外的模板参数，有默认值 0**”这个位置：

- 如果条件为真，`std::enable_if_t<true, int> == int`，这个参数等价于 `int = 0`，合法；
- 如果条件为假，`std::enable_if_t<false, int>` 代入失败 → SFINAE：整个模板版本被丢弃。

---

### 2.3 用在类模板 / 成员函数上

类模板：

```cpp
template<class T,
         class = std::enable_if_t<std::is_integral_v<T>>>
struct OnlyIntegral {
    T value;
};
```

成员函数：

```cpp
struct X {
    template<class T>
    std::enable_if_t<std::is_integral_v<T>, void>
    set(T v) {
        // 只接受整型
    }
};
```

思路完全一样：**在哪个位置写 `enable_if_t`，就在哪个位置触发“代入失败 → 把这个重载/特化踢掉”。**

---

## 3. 你实际上需要掌握到什么程度？

结合你现在的目标（“业务为主，要应付面试”），我建议的掌握度是：

1. **概念能说清楚**（面试口头版）：

   > `std::enable_if` 是 `<type_traits>` 里的一个模板，它根据编译期布尔条件决定是否提供某个类型别名。我们一般通过 `std::enable_if_t<条件, 类型>` 来用它：条件为真时，这是一个合法类型；条件为假时，模板参数替换失败，按 SFINAE 规则把对应的函数模板/类模板候选丢弃。  
   > 常用来“开/关”某些重载，比如“只有整型才启用这一版函数”。

2. **能看懂/手写简单模式**：

   至少会写一个这样的函数：

   ```cpp
   template<class T>
   std::enable_if_t<std::is_integral_v<T>, void>
   print_if_integral(T x) {
       std::cout << x << "\n";
   }
   ```

   以及知道如果加个兜底版，非整型会落到哪里去。

3. **知道有 `enable_if_t` 这个别名**：

   实际写业务时，都是用 `_t` 版本，不自己写 `typename std::enable_if<...>::type` 这一长串。

---

## 4. 小结（给你一个“记忆锚点”）

- 真正的 `enable_if` 就在 `<type_traits>` 里，声明形式和我手写的版本等价。
- 它的核心作用就是：**把一个 `bool` 条件变成“有无某个类型”的开关**。
- 和之前你刚学懂的 SFINAE 连接起来就是：
  - 条件为假 → 这个类型不存在 → 在函数声明处代入失败 → 这条模板候选被 SFINAE 丢弃；
  - 条件为真 → 代入成功 → 这条模板候选参与后面的重载决议。

# void_t

好，我们专门把 `std::void_t` 和这段 `has_size` 拆开讲一遍，用**超慢动作**。

先把代码贴一下：

```cpp
// 默认：没 size()
template<class, class = void>
struct has_size : std::false_type {};

// 如果下面这行能成功替换，就偏特化成功，value = true
template<class T>
struct has_size<T,
    std::void_t<decltype(std::declval<T>().size())>
> : std::true_type {};
```

---

## 1. `std::void_t` 本体到底是什么？

头文件 `<type_traits>` 里面，大致是这样定义的（简化版）：

```cpp
template<class...>
using void_t = void;
```

含义非常朴素：

- 不管给它多少模板参数（0 个、1 个、N 个），
- 只要这些参数**本身在模板参数替换时是合法的**，
- `void_t<...>` 的结果**永远就是 `void`**。

关键点：  
**如果参数里某个东西不合法，整个 `void_t<...>` 这行就“代入失败” → 触发 SFINAE。**

---

## 2. 回到 `has_size`：它整体想干嘛？

这两个模板一起，实现一个**编译期判断**：

> `has_size<T>::value`：  
> - 如果 `T` 有成员函数 `size()`，就为 `true`；  
> - 否则为 `false`。

也就是一个“有无 `size()`”的 trait（类型性质标记）。

---

## 3. 先看“默认版本”：假设都没有 `size()`

```cpp
template<class, class = void>
struct has_size : std::false_type {};
```

拆开讲：

- 这是 `has_size` 的**主模板**。
- 有两个模板参数：
  - 第一个是“随便什么类型”，不用名字；
  - 第二个叫 `class = void`，有一个默认参数 `void`。
- 继承自 `std::false_type`：  
  `std::false_type` 里有一个 `static constexpr bool value = false;`。

**含义：**

> 除非有更匹配的偏特化，否则：  
> 对所有类型 `T`，`has_size<T>` 默认就是 `false_type`，也就是 `has_size<T>::value == false`。

你可以先认为：  
> “默认当作没有 `size()`。”

---

## 4. 再看偏特化：当 T 有 `.size()` 时变成 true

```cpp
template<class T>
struct has_size<T,
    std::void_t<decltype(std::declval<T>().size())>
> : std::true_type {};
```

这一段就是**条件：如果能写出 `decltype(std::declval<T>().size())`，就启用这个版本**。

逐个拆：

1. `template<class T>`  
   这是一个对前面 `has_size` 的**偏特化**版本。

2. `struct has_size<T, std::void_t< ... >>`  
   说明“当第二个模板参数**恰好等于** `std::void_t<某个东西>`”时，使用这一版。

3. 关键部分：

   ```cpp
   std::void_t<decltype(std::declval<T>().size())>
   ```

   - `std::declval<T>()`：一个工具，用来在**不真的构造对象**的情况下**得到一个 `T&&` 表达式**，方便放到 `decltype` 里面。  
     记成“假装有一个 T 类型的东西”。
   - `std::declval<T>().size()`：  
     如果 `T` 有成员函数 `size()`，这个表达式**是合法的**；  
     如果没有，表达式不合法。
   - `decltype( ... )`：取这个表达式的类型。
   - 整体：`std::void_t<那个decltype>`：
     - 如果 `decltype(...)` 表达式本身合法，那么 `void_t<...>` 结果就是 `void`；
     - 如果 `decltype(...)` 不合法（因为 `T` 没有 `size()`），**在给这个模板参数赋值时就会“代入失败”**。

4. `: std::true_type {}`  
   当这个偏特化成功匹配时，`has_size<T, ...>` 继承自 `std::true_type`，意味着：
   - `has_size<T>::value == true`。

---

## 5. 具体走一遍：对有 `size()` 和没有 `size()` 的类型

### 5.1 情况一：`T = std::vector<int>`

- 首先看有没有偏特化能匹配：
  - `has_size<T, std::void_t<decltype(std::declval<T>().size())>>`  
    这里尝试替换 `T = std::vector<int>`：
    - 表达式 `std::declval<std::vector<int>>().size()` 合法；
    - 它的类型是 `std::size_t`；
    - `std::void_t<这个 std::size_t>` 成功 → 结果是 `void`；
    - 所以整个模板参数列表变成 `has_size<std::vector<int>, void>`，偏特化**匹配成功**。
- 因此，对 `std::vector<int>`，使用偏特化版本：
  ```cpp
  struct has_size<std::vector<int>, void> : std::true_type {};
  ```
  → `has_size<std::vector<int>>::value == true`。

### 5.2 情况二：`T = int`

- 尝试偏特化：
  - 需要构造 `has_size<int, std::void_t<decltype(std::declval<int>().size())>>`；
  - 但是 `std::declval<int>().size()` 这个表达式**不合法**（`int` 没有 `size`）；
  - 在替换第二个模板参数 `class = std::void_t<...>` 的时候就**代入失败**；
  - 于是**整条偏特化被 SFINAE 丢弃**，不当候选。
- 只能退回到主模板：

  ```cpp
  template<class, class = void>
  struct has_size : std::false_type {};
  ```

  第一个模板参数就是 `int`，第二个默认成 `void` → `has_size<int, void>`，继承自 `false_type`。

  → `has_size<int>::value == false`。

---

## 6. 再用一句“傻子版”总结 `std::void_t` 在这里干了啥

> `std::void_t<某个表达式>` 的作用是：  
> “**只要这个表达式是合法的，就给我一个 `void` 类型；如果表达式不合法，就在模板参数替换这一步挂掉，触发 SFINAE**。”

在 `has_size` 中：

- 我们**不关心** `.size()` 的返回类型到底是什么；
- 我们只在乎：  
  - “写 `T().size()` 这句话是**不是合法的 C++ 代码**”；
- 所以用 `void_t` 把它“压扁成 `void`”，只利用它“合法 / 不合法”的这一位信息。

---

## 7. 控制掌握程度（你真的只需要到哪？）

对于你现在这个阶段，我建议把 `std::void_t` 理解到：

- 它是 `<type_traits>` 提供的一个别名模板：`template<class...> using void_t = void;`
- **用法**：
  - 写一个 trait 的**偏特化**；
  - 在偏特化参数里放 `std::void_t<表达式>`；
  - 当这个表达式对某个 `T` 合法时，偏特化启用 → `value = true`；
  - 不合法时，这个偏特化被 SFINAE 丢弃 → 退回主模板 → `value = false`。

你可以先把这句记到文档旁边当“口诀”：

> `void_t` = “表达式合法就变成 `void`，不合法就触发 SFINAE 的检测工具”。


# 重载决议和 ADL

## 1. 重载决议：模板/普通函数 + SFINAE 之后的“选谁”

你不需要背特别细的标准规则，只要有**直觉模型**就够应付大多数面试。

可以记成大致三层：

1. **先收集候选**：
   - 普通函数；
   - 函数模板（对每个模板尝试推导参数、做替换）；
   - 这时 SFINAE 会先把“不合法的模板候选”踢掉。

2. **选“可行的”候选**：
   - 参数个数匹配；
   - 类型可以转换（有隐式转换等）。

3. **在可行的里挑“最匹配”的一个”**（大白话版本）：
   - 精确匹配优于需要转换的；
   - 非模板优于模板；
   - 更特化的模板优于更泛的模板；
   - 若考虑 const/引用等，`T&` > `const T&` > 按值接收。

举个 SFINAE 参与后的效果：

```cpp
template<class T>
std::enable_if_t<std::is_integral_v<T>, void>
f(T);  // 整型版

template<class T>
std::enable_if_t<!std::is_integral_v<T>, void>
f(T);  // 非整型版
```

- 对 `f(10)`：  
  - 第一个模板 `T=int`：`is_integral_v<int> = true` → 保留；  
  - 第二个模板 `T=int`：`!is_integral_v<int> = false` → SFINAE 丢弃；  
  - 最终候选集中**只有“整型版”**，重载决议自然选它。
- 对 `f(3.14)`：反过来，保留第二个，丢弃第一个。

这就是“重载决议 + SFINAE”配合起来的实际效果。

---
## 2. ADL（Argument-Dependent Lookup）：根据实参类型多看几个命名空间

名字很吓人，本质很朴素：

> 调用一个**非限定名函数**（没写命名空间前缀，比如 `swap(a, b)`），
> 编译器除了在当前作用域找，还会**额外在“实参类型所在的命名空间”里找重载**。

### 2.1 为什么要这么干？

场景：你写了一个自己的类型 `ns::X`，并且为它提供了一个**自定义的非成员 `swap`**：

```cpp
namespace ns {

struct X { /* ... */ };

void swap(X& a, X& b) {
    std::cout << "ns::swap(X,X)\n";
    // 自己实现高效 swap
}

} // namespace ns
```

在另一个地方，用标准库算法：

```cpp
using std::swap;  // 把 std::swap 带进作用域

ns::X a, b;
swap(a, b);
```

这里 `swap` 没有写 `std::` 或 `ns::`，是一个**非限定名调用**。
编译器怎么决定是 `std::swap` 还是 `ns::swap`？

* 先在当前作用域找：
  因为有 `using std::swap;`，所以有 `std::swap` 候选。
* 再触发 ADL：
  实参类型是 `ns::X` → 编译器会去命名空间 `ns` 里搜索 `swap`，发现 `ns::swap(X&, X&)` 也是候选。
* 然后在这两个候选里做重载决议：

  * 对 `ns::X` 来说，`ns::swap(X&, X&)` 是完全匹配；
  * `std::swap` 一般是模板，需要从 `T` 推导成 `ns::X`，略逊一筹；
  * 所以最终选 `ns::swap`。

这就是你在 STL 代码里常见写法：

```cpp
using std::swap;
swap(a, b);
```

的原因：

* `using std::swap;` 确保**至少有一个标准版本**在候选里；
* 裸 `swap(a, b)` 触发 ADL，让自定义的 `swap` 有机会被找到并选中。

---

### 2.2 面试怎么说 ADL

> ADL 是参数相关查找：
> 对非限定名的函数调用，比如 `f(x, y)`，编译器除了在当前作用域找 `f`，还会根据实参类型所在的命名空间，去这些命名空间里继续找同名函数，一起参与重载决议。
>
> 常见用法是：库或用户为自己的类型在同一命名空间中定义非成员函数（比如 `swap`），配合
> `using std::swap; swap(a, b);`
> 让 ADL 优先选择自定义版本，同时有标准库实现作为兜底。

