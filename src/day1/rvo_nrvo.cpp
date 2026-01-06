#include <iostream>
#include <utility>

struct Tracer {
    Tracer() { std::cout << "ctor\n"; }
    Tracer(const Tracer&) { std::cout << "copy\n"; }
    Tracer(Tracer&&) noexcept { std::cout << "move\n"; }
    ~Tracer() { std::cout << "dtor\n"; }
};

// 匿名返回（C++17 起强制无 copy/move）
Tracer make_prvalue() {
    return Tracer{}; // RVO: 理论上不打印 copy/move
}

// 具名返回，单一路径，通常触发 NRVO
Tracer make_named() {
    Tracer t;
    return t; // NRVO 允许，主流编译器无 copy/move
}

// 具名返回，多分支，不保证 NRVO
Tracer make_branch(bool cond) {
    Tracer a, b;
    if (cond) return a; // 可能打印 copy/move
    return b;
}

int main() {
    std::cout << "== RVO (anonymous) ==\n";
    auto x = make_prvalue();

    std::cout << "== NRVO (named, single path) ==\n";
    auto y = make_named();

    std::cout << "== NRVO maybe fails (two names) cond=true ==\n";
    auto z1 = make_branch(true);

    std::cout << "== NRVO maybe fails (two names) cond=false ==\n";
    auto z2 = make_branch(false);

    std::cout << "done\n";
    return 0;
}
