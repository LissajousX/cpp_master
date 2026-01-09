## 函数模板实参推导的基本规则

1) 按值形参 `T`，顶层 const 被忽略
- 规则：`const`/`volatile` 顶层限定在按值传参时被去掉，T 按去掉后的类型推导。
- 示例：
  ```cpp
  template<class T>
  void by_value(T x); // 按值

  const int ci = 1;
  by_value(ci);   // T 推导为 int（顶层 const 去掉）
  by_value(42);   // T 推导为 int
  ```
- 含义：T 只是描述“拷贝后”的类型，不携带顶层 const。

2) 左值引用形参 `T&`
- 规则：只有左值能绑定 `T&`，T 推导为“去掉引用后的类型”，但保留底层 const。
- 示例：
  ```cpp
  template<class T>
  void by_lref(T& x);

  int a = 0;
  const int ca = 0;
  by_lref(a);   // T = int
  by_lref(ca);  // T = const int  （底层 const 保留）
  // by_lref(42); // ❌ 右值无法绑定 T&，编译失败
  ```
- 含义：可区分可变/不可变左值。

3) 左值 const 引用形参 `const T&`
- 规则：可绑定任意值类别，T 推导时会去掉顶层 const，但底层 const 会体现在 T 上。
- 示例：
  ```cpp
  template<class T>
  void by_const_lref(const T& x);

  int a = 0;
  const int ca = 0;
  by_const_lref(a);   // T = int
  by_const_lref(ca);  // T = int（顶层 const 去掉，因形参已有 const）
  by_const_lref(42);  // T = int
  ```
- 含义：通吃左值/右值，适合只读场景。

4) 形如 `T&&` 且 T 需“推导得到”→ 转发引用，依赖引用折叠
- 规则：如果 `T&&` 来自模板实参推导（或 `auto&&`），它是转发引用；结合引用折叠：
  - 传左值：T 推导为 `X&`，折叠 `X& &&` 得 `X&`
  - 传右值：T 推导为 `X`，折叠得 `X&&`
- 示例：
  ```cpp
  template<class T>
  void forward_ref(T&& x) { /* ... */ }

  int a = 0;
  forward_ref(a);   // T = int&，形参类型为 int&（左值折叠）
  forward_ref(42);  // T = int，形参类型为 int&&（右值）
  ```
- 注意：写死类型的 `int&&` 不是转发引用，只能接右值；类模板里若 T 已实例化，成员函数的 `T&&` 也不是转发引用，除非该成员函数自身是模板并重新推导自身的模板参数。

5) 小结口诀
- 按值：顶层 const 去掉。
- `T&`：只能左值，保留底层 const。
- `const T&`：通吃左/右值，只读。
- `推导得到的 T&&`：转发引用，折叠规则“有 & 则 &，双 && 留 &&”。