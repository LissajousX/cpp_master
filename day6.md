## Day6：OOP 基础复盘 + 设计强化 + 容器与迭代器失效

### 理论速览
- OOP 三大特性：封装（隐藏实现细节，只暴露必要接口）、继承（代码复用 + 接口扩展）、多态（通过虚函数/接口统一操作不同实现）。
- 虚函数表与动态绑定：`virtual` 开启运行时多态；通过基类指针/引用调用时，经由虚表决定实际调用的实现；`override`/`final` 用于防止误写和限制继承。
- 菱形继承与虚继承成本：多条继承路径导致重复基类子对象和二义性；`virtual` 继承将公共基类合并为一个虚基子对象，但会让对象布局和指针调整更复杂。
- OOP 设计原则与取舍：组合 vs 继承（“能用组合就别用继承”）、接口/实现分离、pimpl（隐藏实现细节、降低编译依赖，代价是多一层 indirection）。
- 容器复杂度与迭代器失效：`vector/list/deque/map/unordered_map` 的典型操作复杂度；哪些操作会使迭代器、指针、引用失效；`reserve` 和 `emplace` 对迭代器失效和性能的影响。

### 最小代码示例
```cpp
#include <iostream>
#include <memory>
#include <vector>
#include <string>

// ===== 1. OOP 基础：接口 + 继承 + 多态 =====

struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;        // 纯虚函数：接口
    virtual void draw() const {             // 可选默认实现
        std::cout << "Shape::draw()\n";
    }
};

struct Circle final : Shape {
    double r;
    explicit Circle(double radius) : r(radius) {}

    double area() const override {
        return 3.14159 * r * r;
    }

    void draw() const override {
        std::cout << "Circle, r=" << r << " area=" << area() << "\n";
    }
};

struct Rectangle : Shape {
    double w, h;
    Rectangle(double w_, double h_) : w(w_), h(h_) {}

    double area() const override { return w * h; }

    void draw() const override {
        std::cout << "Rectangle, " << w << "x" << h << " area=" << area() << "\n";
    }
};

inline void draw_all(const std::vector<std::unique_ptr<Shape>>& shapes) {
    for (auto const& p : shapes) {
        p->draw();   // 通过基类指针触发动态绑定
    }
}

// ===== 2. 菱形继承 + 虚继承示例（展示布局与调用成本） =====

struct Base {
    int value = 0;
    virtual void who() const {
        std::cout << "Base::who, value=" << value << "\n";
    }
    virtual ~Base() = default;
};

struct Left : virtual Base {
    Left() { value = 1; }
    void who() const override {
        std::cout << "Left::who, value=" << value << "\n";
    }
};

struct Right : virtual Base {
    Right() { value = 2; }
    void who() const override {
        std::cout << "Right::who, value=" << value << "\n";
    }
};

struct Diamond final : Left, Right {
    Diamond() {
        // 虚继承下只有一个 Base 子对象
        value = 42;
    }
    void who() const override {
        std::cout << "Diamond::who, value=" << value << "\n";
    }
};

inline void test_diamond() {
    std::cout << "sizeof(Base)    = " << sizeof(Base)    << "\n";
    std::cout << "sizeof(Left)    = " << sizeof(Left)    << "\n";
    std::cout << "sizeof(Right)   = " << sizeof(Right)   << "\n";
    std::cout << "sizeof(Diamond) = " << sizeof(Diamond) << "\n";

    Diamond d;
    Base* pb = &d;   // 指向唯一的虚基 Base
    pb->who();       // 动态绑定
}

// ===== 3. 容器与迭代器失效简要示例 =====

inline void iterator_invalidation_demo() {
    std::vector<int> v{1, 2, 3};
    auto it = v.begin();
    std::cout << "before: *it=" << *it << " size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // 可能触发重分配，导致 it 失效
    v.push_back(4);
    std::cout << "after push_back: size=" << v.size() << " capacity=" << v.capacity() << "\n";

    // *it 这里是否安全，取决于是否发生了重分配（面试常考点）
}

int main() {
    std::cout << "== OOP 多态基础 ==\n";
    std::vector<std::unique_ptr<Shape>> shapes;
    shapes.emplace_back(new Circle(1.0));
    shapes.emplace_back(new Rectangle(2.0, 3.0));
    draw_all(shapes);

    std::cout << "\n== 菱形继承 + 虚继承 ==\n";
    test_diamond();

    std::cout << "\n== 容器与迭代器失效（示意） ==\n";
    iterator_invalidation_demo();
}
```

> 说明：真正实现时，可以在 `src/day6` 目录下拆分为多个示例文件（如 `oop_shapes.cpp`、`diamond_virtual.cpp`、`iterator_invalidation.cpp`），本文件只展示你在 Day6 需要掌握的核心点。

### 练习题
1) 设计一个 `Shape` 层次的变体：增加 `Triangle`，并让 `draw_all` 只依赖基类接口就能正确输出。思考：哪些成员应该放在基类，哪些留在派生类？
2) 写一个**不使用虚继承**的菱形继承例子：`Base <- Left`、`Base <- Right`、`Diamond : Left, Right`，演示：
   - 同名 `Base::value` 出现两份时的二义性；
   - 改为虚继承后如何消除这个问题，并比较 `sizeof`。 
3) 针对一个你熟悉的业务概念（比如订单、任务、消息），设计一个简单的类层次：
   - 什么时候用继承？什么时候用组合？
   - 哪些接口应该是虚函数？哪些可以是非虚成员？
4) 写出 `vector`、`list`、`deque`、`map`、`unordered_map` 中以下操作的大致复杂度：
   - 头插、尾插、中间插入；
   - 按 key 查找 / 按值线性搜索。  
   并整理“哪些操作会使这些容器的迭代器失效”成一张小表。
5) 对同一段“插入 N 个元素”的代码，分别用 `push_back` 和先 `reserve` 再 `push_back` 对比时间（或者至少打印重分配次数），理解预留容量对性能和迭代器失效的影响。

### 自测要点（回答并尽量用自己的话复述）
- OOP 三大特性分别是什么？在 C++ 代码里是通过哪些语法手段体现的（访问控制、class/struct、继承、虚函数等）？
- `virtual` / `override` / `final` 各自的语义是什么？在哪些场景下强烈建议加上 `override`，在哪些时候会考虑使用 `final`？
- 菱形继承在没有虚继承时会产生什么问题？虚继承是如何把多个基类子对象“合并”为一个虚基子对象的？代价体现在哪些方面（对象布局、指针调整、间接访问）？
- 在实际项目中，什么时候更倾向于“用组合代替继承”？有哪些你见过/能想象到的“滥用继承”的例子？
- `vector` / `list` / `deque` / `map` / `unordered_map` 的核心特性和典型使用场景分别是什么？它们在插入、删除、查找上的复杂度大致如何？
- 什么是“迭代器失效”？请举出 `vector` 和 `unordered_map` 中各一个会导致迭代器失效的操作，并说明如何规避或应对。
