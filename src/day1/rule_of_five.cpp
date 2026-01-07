#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

// 展示 1：自定义移动导致拷贝被删除（需显式写拷贝才可用）
struct MoveOnly {
    MoveOnly() = default;
    MoveOnly(MoveOnly&&) noexcept { std::cout << "MoveOnly move ctor\n"; }
    MoveOnly& operator=(MoveOnly&&) noexcept {
        std::cout << "MoveOnly move assign\n";
        return *this;
    }
    // 未声明拷贝，因存在自定义移动，拷贝被删除。
};

// 展示 2：自定义拷贝会抑制隐式移动，需要显式写移动
struct CopyFirst {
    CopyFirst() = default;
    CopyFirst(const CopyFirst&) { std::cout << "CopyFirst copy ctor\n"; }
    CopyFirst& operator=(const CopyFirst&) {
        std::cout << "CopyFirst copy assign\n";
        return *this;
    }
    // 想要移动，必须显式写出：
    CopyFirst(CopyFirst&&) noexcept { std::cout << "CopyFirst move ctor\n"; }
    CopyFirst& operator=(CopyFirst&&) noexcept {
        std::cout << "CopyFirst move assign\n";
        return *this;
    }
};

// 展示 3：完整 Rule of Five（含析构，移动标 noexcept，copy-and-swap 赋值）
struct Buffer {
    Buffer() = default;
    ~Buffer() = default;

    Buffer(const Buffer& other) : data(other.data) { std::cout << "Buffer copy ctor\n"; }
    Buffer(Buffer&& other) noexcept : data(std::move(other.data)) { std::cout << "Buffer move ctor\n"; }

    Buffer& operator=(Buffer other) noexcept { // 兼容拷/移
        std::cout << "Buffer copy/move assign via swap\n";
        swap(other);
        return *this;
    }

    void swap(Buffer& o) noexcept { data.swap(o.data); }

private:
    std::vector<int> data{1,2,3};
};

int main() {
    static_assert(!std::is_copy_constructible_v<MoveOnly>, "MoveOnly: copy deleted");
    static_assert(std::is_move_constructible_v<MoveOnly>, "MoveOnly: move ok");

    std::cout << "== MoveOnly（自定义移动，拷贝被删除） ==\n";
    MoveOnly m1;
    MoveOnly m2 = std::move(m1);
    MoveOnly m3;
    m3 = std::move(m2);
    // MoveOnly m4 = m3; // 拷贝会编译失败，可尝试取消注释

    std::cout << "\n== CopyFirst（自定义拷贝，显式补上移动） ==\n";
    CopyFirst c1;
    CopyFirst c2 = c1;            // copy ctor
    CopyFirst c3 = std::move(c1);  // move ctor（因显式写出）
    c2 = c3;                       // copy assign
    c2 = std::move(c3);            // move assign

    std::cout << "\n== Buffer（Rule of Five，copy-and-swap） ==\n";
    Buffer b1;
    Buffer b2 = b1;               // copy ctor
    Buffer b3 = std::move(b1);    // move ctor
    b2 = b3;                      // copy assign
    b2 = std::move(b3);           // move assign (via swap)

    return 0;
}
