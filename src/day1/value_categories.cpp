#include <iostream>
#include <type_traits>
#include <utility>

struct Widget {
    int v{};
};

// 重载用于观察值类别命中哪一个
const char* which(Widget&) { return "T&"; }
const char* which(const Widget&) { return "const T&"; }
const char* which(Widget&&) { return "T&&"; }

// 打印 decltype((expr)) 是否为左值/右值引用
#define SHOW(expr) \
    std::cout << #expr << " -> lvalue_ref:" << std::is_lvalue_reference<decltype((expr))>::value \
              << " rvalue_ref:" << std::is_rvalue_reference<decltype((expr))>::value << "\n"

Widget make_value() { return Widget{42}; }            // 返回 prvalue
Widget& make_lref(Widget& w) { return w; }            // 返回 lvalue ref
Widget&& make_rref(Widget&& w) { return std::move(w); } // 返回 xvalue

template <class T>
const char* forward_to_which(T&& t) {
    return which(std::forward<T>(t));
}

int main() {
    Widget w{1};
    const Widget cw{2};

    std::cout << "which(w): " << which(w) << "\n";                 // lvalue
    std::cout << "which(cw): " << which(cw) << "\n";               // const lvalue
    std::cout << "which(std::move(w)): " << which(std::move(w)) << "\n"; // xvalue
    std::cout << "which(make_value()): " << which(make_value()) << "\n"; // prvalue -> binds to T&&

    std::cout << "forward w: " << forward_to_which(w) << "\n";              // 保持 lvalue
    std::cout << "forward move(w): " << forward_to_which(std::move(w)) << "\n"; // 保持 xvalue

    // decltype((expr)) 检查
    SHOW(w);            // lvalue
    SHOW(std::move(w)); // xvalue
    SHOW(make_value()); // prvalue 表达式，decltype((...)) 视为 prvalue -> 非引用
    SHOW(make_lref(w)); // lvalue
    SHOW(make_rref(std::move(w))); // xvalue

    return 0;
}
