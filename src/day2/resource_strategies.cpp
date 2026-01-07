#include <iostream>
#include <memory>
#include <utility>

// 独占资源：不可拷贝，可移动（noexcept）
struct Exclusive {
    explicit Exclusive(int id) : id(id) { std::cout << "Exclusive acquire " << id << "\n"; }
    ~Exclusive() { std::cout << "Exclusive release " << id << "\n"; }

    Exclusive(const Exclusive&) = delete;
    Exclusive& operator=(const Exclusive&) = delete;

    Exclusive(Exclusive&& other) noexcept : id(other.id) {
        other.id = -1;
        std::cout << "Exclusive move ctor\n";
    }
    Exclusive& operator=(Exclusive&& other) noexcept {
        std::cout << "Exclusive move assign\n";
        if (this != &other) {
            id = other.id;
            other.id = -1;
        }
        return *this;
    }

    int id;
};

// 共享资源：shared_ptr，拷贝共享，移动转移
struct Shared {
    explicit Shared(int v) : ptr(std::make_shared<int>(v)) {
        std::cout << "Shared new ref " << *ptr << " use_count=" << ptr.use_count() << "\n";
    }
    int value() const { return *ptr; }
    void set(int v) { *ptr = v; }
    std::shared_ptr<int> ptr;
};

// 可拷可移：显式 Rule of Five，移动 noexcept，copy-and-swap 赋值
struct Hybrid {
    Hybrid() = default;
    explicit Hybrid(int v) : data(new int(v)) {
        std::cout << "Hybrid ctor " << *data << "\n";
    }
    ~Hybrid() { reset(); }

    Hybrid(const Hybrid& o) : data(o.data ? new int(*o.data) : nullptr) {
        std::cout << "Hybrid copy ctor\n";
    }
    Hybrid& operator=(Hybrid o) noexcept { // copy-and-swap, strong guarantee
        std::cout << "Hybrid copy/move assign\n";
        swap(o);
        return *this;
    }

    Hybrid(Hybrid&& o) noexcept : data(o.data) {
        o.data = nullptr;
        std::cout << "Hybrid move ctor\n";
    }

    void swap(Hybrid& o) noexcept { std::swap(data, o.data); }
    void reset() noexcept {
        if (data) {
            std::cout << "Hybrid dtor freeing " << *data << "\n";
            delete data;
            data = nullptr;
        }
    }
    int get() const { return data ? *data : -1; }

private:
    int* data{};
};

int main() {
    std::cout << "== 独占资源（不可拷，可移） ==\n";
    Exclusive ex1(1);
    Exclusive ex2 = std::move(ex1);
    Exclusive ex3(3);
    ex3 = std::move(ex2);

    std::cout << "\n== 共享资源（shared_ptr） ==\n";
    Shared s1(42);
    Shared s2 = s1; // 共享所有权
    std::cout << "s1 use_count=" << s1.ptr.use_count() << " s2 val=" << s2.value() << "\n";
    s2.set(100);
    std::cout << "after set, s1 val=" << s1.value() << " s2 val=" << s2.value() << " use_count=" << s1.ptr.use_count() << "\n";

    std::cout << "\n== 可拷可移（显式 Rule of Five） ==\n";
    Hybrid h1(7);
    Hybrid h2 = h1;          // copy
    Hybrid h3 = std::move(h1); // move
    h2 = h3;                 // copy assign via swap
    h3 = Hybrid(9);          // move assign via swap

    return 0;
}
