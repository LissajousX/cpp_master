#include <iostream>
#include <utility>

void sink(int& x) {
    std::cout << "sink(int&)  -> lvalue\n";
}

void sink(int&& x) {
    std::cout << "sink(int&&) -> rvalue\n";
}

template<typename T>
void level1_good(T&& x) {
    std::cout << "level1_good -> ";
    sink(std::forward<T>(x));
}

template<typename T>
void level2_good(T&& x) {
    std::cout << "level2_good -> ";
    level1_good(std::forward<T>(x));
}


template<typename T>
void level1_bad(T&& x) {
    std::cout << "level1_bad  -> ";
    sink(std::forward<T&&>(x));   // ❌ 错误写法
}

template<typename T>
void level2_bad(T&& x) {
    std::cout << "level2_bad  -> ";
    level1_bad(std::forward<T&&>(x));
}

int main() {
    int a = 42;

    std::cout << "=== direct ===\n";
    sink(a);
    sink(10);

    std::cout << "\n=== good forwarding ===\n";
    level2_good(a);
    level2_good(10);

    std::cout << "\n=== bad forwarding ===\n";
    level2_bad(a);
    level2_bad(10);
}
