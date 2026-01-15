# C++ 基础面试评估试卷（进阶·偏实操）
建议用时：**180 分钟**  
满分：**100 分**  
说明：尽量使用 **C++17**；除非题目明确允许，否则请避免使用第三方库。编程题以“可读性 + 正确性 + 异常安全 + 资源管理”综合评分。

---

## 一、代码阅读与改错（共 30 分）

### 题 1：拷贝/析构/异常安全（10 分）
阅读代码，回答问题并给出修复版本（可只写类定义与关键函数实现）。

```cpp
#include <cstring>
#include <utility>

class Buffer {
public:
    Buffer(size_t n)
        : n_(n), p_(new char[n]) {}

    Buffer(const char* s) {
        n_ = std::strlen(s);
        p_ = new char[n_];
        std::memcpy(p_, s, n_);
    }

    ~Buffer() { delete p_; }

    Buffer(const Buffer& other) {
        n_ = other.n_;
        p_ = other.p_;
    }

    Buffer& operator=(const Buffer& other) {
        if (this == &other) return *this;
        delete[] p_;
        n_ = other.n_;
        p_ = new char[n_];
        std::memcpy(p_, other.p_, n_);
        return *this;
    }

    char* data() { return p_; }
    const char* data() const { return p_; }
    size_t size() const { return n_; }

private:
    size_t n_{};
    char* p_{};
};
```

要求：
1. 指出至少 **4 个问题**（包含但不限于：`delete/delete[]`、浅拷贝、越界/缺少终止符、异常安全、强保证等）。
2. 给出一个修复方案，满足：
   - **Rule of 5 或 Rule of 0**
   - 拷贝赋值满足**强异常安全保证**（提示：copy-and-swap）
   - `data()` 的 `const`/非 `const` 接口合理

---

### 题 2：迭代器失效与未定义行为（10 分）
阅读代码并回答：哪些行可能导致 UB 或逻辑错误？为什么？如何改？

```cpp
#include <vector>
#include <iostream>

int main() {
    std::vector<int> v{1,2,3,4,5};

    auto it = v.begin();
    v.push_back(6);
    std::cout << *it << "\n";

    for (auto it2 = v.begin(); it2 != v.end(); ++it2) {
        if (*it2 % 2 == 0) {
            v.erase(it2);
        }
    }

    for (int x : v) std::cout << x << " ";
}
```

要求：
1. 明确指出“哪一段”为什么会出现**迭代器失效**。
2. 给出两种改法：
   - 用正确的 `erase` 循环模式
   - 用算法（如 `remove_if` + `erase`）

---

### 题 3：多态删除与对象切片（10 分）
阅读代码并回答问题：

```cpp
#include <iostream>

struct Base {
    void f() { std::cout << "Base::f\n"; }
    ~Base() { std::cout << "~Base\n"; }
};

struct Derived : Base {
    void f() { std::cout << "Derived::f\n"; }
    ~Derived() { std::cout << "~Derived\n"; }
};

void g(Base b) {
    b.f();
}

int main() {
    Base* p = new Derived();
    p->f();
    g(*p);
    delete p;
}
```

要求：
1. 写出程序输出（允许说明“某行未定义行为/不确定”）。
2. 指出至少 **3 个设计问题**（动态绑定条件、虚析构、多态接口、对象切片）。
3. 给出更合理的接口设计（函数签名、`virtual`、`override` 等）。

---

## 二、实操编程题（共 60 分）

### 题 4：实现 `SmallVector` 的核心能力（25 分）
实现一个最小可用的 `SmallVector<T, N>`（小对象优化：前 `N` 个元素存放在栈内 buffer，超过后转堆）。

要求（重点评分点）：
- 支持：默认构造、析构、`push_back(const T&)`、`push_back(T&&)`、`size()`、`capacity()`、`operator[]`、`begin()/end()`
- 正确处理：
  - **非平凡类型**（有析构/拷贝/移动）
  - 扩容时优先移动（若可行）
  - 异常安全：至少保证**基本保证**，加分项：关键路径做到强保证
- 禁止使用 `std::vector<T>` 存储元素（可以用 `std::allocator<T>` 或 `::operator new` + placement new）

提示：你可以只实现必要的特殊成员函数；不要求完整 STL 兼容，但必须能正确析构所有构造过的元素。

附：建议你写的最小测试点（不要求提交测试文件，但实现应能通过）：
- `SmallVector<int, 4>` 连续 `push_back` 1..10
- `SmallVector<std::string, 2>` 推入/扩容/销毁不崩溃
- 自定义类型统计构造/析构次数，确保无泄漏、无重复析构

---

### 题 5：实现线程安全的计数器 + 性能对比解释（15 分）
实现 `Counter`，提供：
- `void inc();`
- `long long value() const;`

并分别实现两种版本：
1. 用 `std::mutex` 保护
2. 用 `std::atomic<long long>`

要求：
1. 给出你对两种实现的**正确性依据**（数据竞争、可见性、原子性）。
2. 解释：为什么在某些场景下 `atomic` 可能比 `mutex` 快，但也可能更慢？（提示：竞争、cache line、伪共享、上下文切换）。
3. 加分：说明 `memory_order_relaxed` 在计数器场景可否使用，使用条件是什么。

---

### 题 6：写一个“不会泄漏资源”的解析函数（20 分）
实现函数：

```cpp
#include <string>
#include <vector>

struct Item {
    std::string key;
    int value;
};

// 输入形如："a=1;b=2;c=3"，分号分隔
// 允许空格："a = 1 ; b=2"
// 遇到非法格式（缺 '='、非整数、key 为空等）要返回 false
bool parse_kv(const std::string& s, std::vector<Item>& out);
```

要求：
- `out` 只有在解析成功时才被修改（强保证）
- 不允许使用异常作为控制流（可以抛异常但要捕获转为 false；更推荐不用抛）
- 复杂度：O(n)

加分项：
- 处理整数溢出
- 允许尾部分号（如 `"a=1;"`）并定义你的行为

---

## 三、工程化与面试追问（共 10 分）

### 题 7：你会如何把这段代码写进生产环境？（10 分）
给你一个“看起来能跑”的类：

```cpp
class X {
public:
    X(int n) : n_(n), p_(new int[n]) {}
    ~X() { delete[] p_; }

    int* data() { return p_; }

private:
    int n_;
    int* p_;
};
```

请回答（尽量结构化）：
1. 你会补哪些特殊成员函数/删除哪些操作？为什么？
2. 你会如何改造以提升异常安全/可维护性？（如用 `std::vector<int>`/`unique_ptr<int[]>`）
3. 你会写哪些测试用例？至少列 5 条（包含边界、异常路径、性能）。

---

# 评分参考（不给出完整标准答案，只给评估维度）
- **资源管理**：是否正确使用 `delete[]`、是否避免浅拷贝、是否满足 Rule of 3/5/0
- **异常安全**：是否理解基本保证/强保证；是否能用 copy-and-swap 或局部临时变量实现强保证
- **容器/迭代器**：是否准确识别迭代器失效；是否给出正确模式
- **多态**：是否理解虚析构、动态绑定、切片问题
- **实操能力**：实现是否可编译、边界是否覆盖、是否考虑非平凡类型与析构

---

## 完成状态
已生成更难、更偏实操的一套试卷，文件：`docs/week1_exam_hard.md`。
