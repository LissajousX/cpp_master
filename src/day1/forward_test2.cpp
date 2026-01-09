#include <iostream>
#include <utility>

struct X {
    X() = default;
    X(const X&) {
        std::cout << "copy\n";
    }
    X(X&&) noexcept {
        std::cout << "move\n";
    }
};
template<typename T>
void sink(T&& x) {
    X y = std::forward<T>(x);
}
template<typename T>
void good(T&& x) {
    sink(std::forward<T>(x));
}
template<typename T>
void bad(T&& x) {
    sink(std::forward<T&&>(x));  // ❌ 错误写法
}
int main() {
    std::cout << "=== good ===\n";
    X a;
    good(a);          // copy
    good(X{});        // move

    std::cout << "\n=== bad ===\n";
    X b;
    bad(b);           // ❌ 这里会出问题
    bad(X{});         // move
}
