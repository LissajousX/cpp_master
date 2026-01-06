#include <iostream>
#include <type_traits>
#include <utility>

// 小工具：判断表达式值类别
#define SHOW(expr) \
    std::cout << #expr << " -> lvalue_ref:" << std::is_lvalue_reference<decltype((expr))>::value \
              << " rvalue_ref:" << std::is_rvalue_reference<decltype((expr))>::value << "\n"

void f(int&)  { std::cout << "f: lvalue\n"; }
void f(int&&) { std::cout << "f: rvalue\n"; }

// 1) 引用折叠 + 转发引用示例
// 调用者传什么值类别进来，call 就保持什么传给 f。
template<class T>
void call(T&& x) { // 这是转发引用（T 由推导得到）
    f(std::forward<T>(x));
}

// 2) 泛型工厂：保持参数值类别交给 T 的构造
struct Holder {
    Holder(int& a, int&& b) {
        std::cout << "Holder ctor: a(lvalue)=" << a << " b(rvalue)=" << b << "\n";
    }
};

template<class T, class... Args>
T make_forward(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

int main() {
    int a = 10;

    std::cout << "== 引用折叠 & 转发引用 ==\n";
    call(a);      // 应命中 f(int&)
    call(42);     // 应命中 f(int&&)

    std::cout << "\n== decltype((expr)) 值类别检查 ==\n";
    SHOW(a);            // lvalue_ref=1
    SHOW(std::move(a)); // rvalue_ref=1

    std::cout << "\n== 泛型工厂（保留值类别） ==\n";
    auto h = make_forward<Holder>(a, 99); // a 以左值传递，99 以右值传递

    return 0;
}
