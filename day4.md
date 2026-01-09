## Day4：模板与类型推导（C++11→17）

### 理论速览
- 模板实参推导：函数模板/类模板实参如何从调用处推导；引用折叠规则在 `T&&`（转发引用）中的作用。
- `auto`/`decltype`/`decltype(auto)`：类型推导差异；`decltype((expr))` 保留值类别，`decltype(expr)` 规则（非括号 id 表达式得声明类型）。
- 别名模板（alias template）：用 `using` 定义模板别名，避免 `typedef` 不能模板化的限制。
- C++17 CTAD（Class Template Argument Deduction）：类模板可像函数模板一样推导模板参数，需提供推导指引或依赖已有构造。

### 最小代码示例
```cpp
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>
#include <string>

template<class T>
void info(T&& x) {
    // 打印推导结果与值类别
    std::cout << "T is const? " << std::is_const_v<std::remove_reference_t<T>>
              << ", lvalue? " << std::is_lvalue_reference_v<T>
              << ", rvalue? " << std::is_rvalue_reference_v<T> << "\n";
}

// 别名模板
template<class T>
using Vec = std::vector<T>;

// CTAD 示例：自定义包装
template<class T>
struct Box {
    T value;
    Box(T v) : value(std::move(v)) {}
};
// 推导指引（C++17）：允许 Box b(1) 推导为 Box<int>
Box(int) -> Box<int>;
Box(const char*) -> Box<std::string>;

int main() {
    std::cout << "== auto / decltype / decltype(auto) ==\n";
    int a = 0; const int ca = 1;
    auto x = ca;          // x: int（顶层 const 被去掉）
    const auto y = ca;    // y: const int
    decltype(a) da = a;   // da: int
    decltype((a)) ra = a; // ra: int&（注意双括号保留值类别）

    std::cout << "== 模板实参推导 + 转发引用 ==\n";
    info(a);    // T=int&, lvalue
    info(42);   // T=int, rvalue

    std::cout << "== 别名模板 ==\n";
    Vec<int> v = {1,2,3};
    std::cout << "v size=" << v.size() << "\n";

    std::cout << "== CTAD ==\n";
    Box b1(1);            // 推导为 Box<int>
    Box b2("hi");        // 推导为 Box<std::string>
    std::cout << b1.value << ", " << b2.value << "\n";
}
```

### 练习题
1) 写一个类型打印 helper（如 `type_name<T>()`）用于调试模板推导，结合 `decltype((expr))` 查看值类别。  
2) 编写一个函数模板 `make_pair_like`，演示 `auto` 返回类型与 `decltype(auto)` 返回类型的差别。  
3) 写一个支持 CTAD 的自定义容器/包装类，给出推导指引，展示构造不同实参时的推导结果。  
4) 使用别名模板为 `std::unique_ptr<T, Deleter>` 提供简化别名，配合自定义 deleter。  
5) 用引用折叠规则解释 `T&&` 在左/右值实参下推导出的类型，并用 `static_assert` 验证。  
6) 给出一个因顶层 const 被 auto 去除而引发的 Bug 示例，并修正。  

### 自测要点（回答并举例）
- `auto` 与 `decltype`、`decltype(auto)` 的推导差异是什么？括号对 `decltype` 有何影响？
- 什么是转发引用？引用折叠规则如何作用在 `T&&` 上？
- 别名模板的好处是什么，相比 `typedef` 有何优势？
- CTAD 的作用是什么，如何编写推导指引？何时不应依赖 CTAD（如可读性、复杂构造）？
- 如何在调试模板推导时检查类型和值类别（`std::is_*` traits、`decltype((expr))`、类型打印 helper）？
