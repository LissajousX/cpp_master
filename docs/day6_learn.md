# OOP基础

## 1. 封装：隐藏实现细节，只暴露必要接口

先用“傻话版”：

> 封装 = **把东西包起来**。  
> 外面的人只能通过你给的几个“按钮”（public 函数）来用这个对象，  
> 看不到、也不该去碰里面具体怎么实现的。

在 C++ 里主要通过：

- `class` / `struct`  
- 访问控制：`public` / `protected` / `private`

一个非常典型的小例子：

```cpp
class BankAccount {
public:
    explicit BankAccount(double balance) : balance_(balance) {}

    void deposit(double amount) {
        if (amount < 0) return;   // 内部可以做校验
        balance_ += amount;
    }

    bool withdraw(double amount) {
        if (amount < 0 || amount > balance_) return false;
        balance_ -= amount;
        return true;
    }

    double balance() const { return balance_; }

private:
    double balance_;  // 外部不能直接改，只能通过接口改
};
```

这里的“封装”体现在：

- 外部**看不到** `balance_` 是怎么存的、有没有额外状态；
- 外部**不能直接写** `balance_`，必须走 `deposit/withdraw`，  
  这些函数内部可以做校验、打日志、加锁等等；
- 一旦你想改实现（比如改成“多币种账户”），只要保持接口不变，  
  调用方就不用全项目跟着改。

**在项目里，封装＝控制访问边界 + 稳定接口。**

---

## 2. 继承：代码复用 + 接口扩展

傻话版：

> 继承 = **在一个已有类型的基础上再“加东西”**。  
> 子类“是一个”父类，可以在父类的基础上加字段、加行为、重写部分行为。

典型代码：

```cpp
struct Animal {
    void eat() { std::cout << "eat...\n"; }
    void sleep() { std::cout << "sleep...\n"; }
};

struct Dog : Animal {
    void bark() { std::cout << "wang!\n"; }
};
```

- `Dog` 继承了 `Animal`：
  - 复用了 `eat` / `sleep` 的实现（代码复用）；
  - 自己加了 `bark`（接口扩展）。
- 在使用上：
  ```cpp
  Dog d;
  d.eat();    // 来自 Animal
  d.bark();   // 自己的
  ```

但现代 C++ / 现代设计更强调：

- **“能用组合就不要滥用继承”**：
  - 继承是一种“强耦合”的复用；
  - 倾向于“`has-a` 关系用组合”，“`is-a` 关系且真的要多态时才用继承”。

例如很多时候会更喜欢：

```cpp
class Dog {
public:
    void eat() { impl_.eat(); }
    void sleep() { impl_.sleep(); }
    void bark() { ... }

private:
    Animal impl_;  // 组合，而不是继承
};
```

---

## 3. 多态：通过虚函数 / 接口统一操作不同实现

傻话版：

> 多态 = **“同一个接口，背后可以有很多种实现，运行时再决定用哪一个”**。  
> 你不关心它到底是 Circle 还是 Rectangle，只要知道它是个 Shape，可以 `draw()` 和 `area()`。

在 C++ 里，典型就是“虚函数 + 通过基类指针/引用调用”：

```cpp
struct Shape {
    virtual ~Shape() = default;
    virtual double area() const = 0;  // 纯虚函数：接口
    virtual void draw() const {
        std::cout << "Shape::draw()\n";
    }
};

struct Circle : Shape {
    double r;
    explicit Circle(double radius) : r(radius) {}
    double area() const override { return 3.14159 * r * r; }
    void draw() const override {
        std::cout << "Circle, area=" << area() << "\n";
    }
};

struct Rectangle : Shape {
    double w, h;
    Rectangle(double w_, double h_) : w(w_), h(h_) {}
    double area() const override { return w * h; }
    void draw() const override {
        std::cout << "Rectangle, area=" << area() << "\n";
    }
};

void draw_all(const std::vector<std::unique_ptr<Shape>>& shapes) {
    for (auto const& p : shapes) {
        p->draw();   // 这里就是多态：真正调用哪个 draw，由对象实际类型决定
    }
}
```

- `draw_all` 只依赖 **`Shape` 这个接口**：
  - 不知道（也不关心）具体是 `Circle` 还是 `Rectangle`。
- 运行时，根据 `p` 实际指向的对象类型，走不同的 `draw()` 实现：
  - 这就是“动态多态 / 运行时多态”。

---

## 4. 面试/项目里，你怎么描述这“三大特性”

你可以准备一个简化版说法（下面是可以直接背的风格）：

- **封装**：  
  “通过 `class` 和访问控制把数据和操作打包在一起，隐藏内部实现，只暴露稳定的接口。  
  比如银行账户类把余额设为 `private`，外部只能通过 `deposit/withdraw` 操作，避免直接改字段。”

- **继承**：  
  “在已有类型基础上扩展功能，子类 `is-a` 父类，可以重用父类代码并新增行为。  
  但现代 C++ 更提倡‘组合优先，继承用于需要多态的场景’，避免滥用继承带来的强耦合。”

- **多态**：  
  “通过虚函数和基类接口，让调用方只依赖抽象，不关心具体实现。  
  比如 `Shape` 定义虚函数 `draw/area`，`Circle/Rectangle` 实现之，容器里存 `Shape*`，统一调用 `draw_all` 即可。”

---

# 虚函数与动态绑定

分三块讲：`virtual` / 虚函数表+动态绑定 / `override` & `final`。

---

## 1. `virtual`：开启“运行时选实现”的开关

有无 `virtual`，行为差异非常大：

```cpp
struct Base {
    void foo()      { std::cout << "Base::foo\n"; }      // 非虚
    virtual void bar() { std::cout << "Base::bar\n"; }   // 虚
};

struct Derived : Base {
    void foo()      { std::cout << "Derived::foo\n"; }
    void bar() override { std::cout << "Derived::bar\n"; }
};

void call(Base* p) {
    p->foo();   // 静态绑定
    p->bar();   // 动态绑定（如果 bar 是 virtual）
}
```

- 若 `Base* p = new Derived;`
  - `p->foo();` 调的还是 `Base::foo`（**非虚：编译期就决定了**）。
  - `p->bar();` 调的是 `Derived::bar`（**虚函数：运行时根据对象真实类型来决定**）。

这就是“运行时多态”：**编译期只知道是 Base 指针，真正调用哪个实现要看运行时对象类型**。

---

## 2. 虚函数表（vtable）与动态绑定：大概是怎么工作的？

实现细节标准没强制规定，但主流编译器都是类似这一套，你理解成“心智模型”就行。

### 2.1 vptr + vtable 的直觉模型

- 每个**含有虚函数的类**，编译器通常会为它生成一张“虚函数表”（vtable）：
  - 这张表里存了所有虚函数在这个类里的**最终实现地址**。
- 每个该类对象内部，通常会隐藏一个指针（vptr），指向它所属类的那张 vtable。

例如：

```cpp
Base b;
Derived d;
Base* p1 = &b;
Base* p2 = &d;
```

可以想象：

- `b` 里有个 vptr 指向“Base 的 vtable”（表里是 `Base::bar` 等函数地址）。
- `d` 里有个 vptr 指向“Derived 的 vtable”（表里 `bar` 项是 `Derived::bar`）。

### 2.2 调用 `p->bar()` 时大致发生啥

```cpp
p->bar();
```

在生成的机器码级别，大致可以想象为：

1. 通过 `p` 取到对象里的 vptr。
2. 通过 vptr 找到该类的 vtable。
3. 在 vtable 里找到 `bar` 对应的那一项函数指针。
4. 间接调用这个函数指针。

- 如果 `p` 指向 `Base` 对象：vptr 指向 Base 的 vtable → 找到的是 `Base::bar`。
- 如果 `p` 指向 `Derived` 对象：vptr 指向 Derived 的 vtable → 找到的是 `Derived::bar`。

所以**同一条代码 `p->bar()`，因为 vptr 不同，最后跳到的实现不同**，就是“动态绑定”。

你可以把它想象成：

> “`p->bar()` 其实是：从一张表里查‘当前这个类的 bar 实现地址’，然后跳过去。”

---

## 3. `override` 和 `final`：让编译器帮你“看门”

### 3.1 `override`：防止“以为重写，实际没重写”

没写 `override` 时，一个很容易出错的地方是：**函数签名不完全匹配**导致没有真正重写，只是“重名了一个新函数”。

例如：

```cpp
struct Base {
    virtual void foo(int);
};

struct Derived : Base {
    void foo(double);   // 少了 override，也没真重写
};
```

- 这里 `Derived::foo(double)` **不是** 对 `Base::foo(int)` 的重写：
  - 只是一个重名的、参数类型不同的重载。
- 用 `Base* p = new Derived; p->foo(42);` 时，真正被调用的还是 `Base::foo(int)`。

加上 `override`：

```cpp
struct Derived : Base {
    void foo(double) override; // 编译错误：没有匹配的虚函数可被 override
};
```

编译器会立刻报错，告诉你“签名对不上，没东西可 override”，防止这种“误以为重写”的 bug。

**建议习惯**：  
只要是打算重写基类虚函数，一律写上 `override`。

### 3.2 `final`：限制“别再继承/别再重写了”

有两种用法：

1. **类上加 `final`：禁止进一步继承**

```cpp
struct Base { virtual void foo(); };

struct Derived final : Base {
    void foo() override;
};

// struct More : Derived {}; // 编译错误：Derived 是 final，不能再继承
```

2. **某个虚函数上加 `final`：禁止更下面的类再重写**

```cpp
struct Base {
    virtual void foo();
};

struct Mid : Base {
    void foo() final;   // 这里是最后一个实现
};

struct Derived : Mid {
    // void foo() override; // 编译错误：Mid::foo 已被标记 final
};
```

这在设计上表达的是：

- “到这一级，这个接口的实现已经固定，不希望再往下层改”。

---

## 4. 你要记住到什么程度？

结合 Day6 的定位，你现在要能做到：

- 用自己的话解释：
  - `virtual` = 开启动态多态，使得通过基类指针/引用调用时，根据对象真实类型走不同实现。
  - vtable/vptr 是主流实现方式：每个含虚函数的类型有一张表，对象里有指针指过去。
- 语义上记清：
  - `override`：保证“我就是在重写基类的某个虚函数”，签名不对就报错。
  - `final`：禁止进一步继承/禁止进一步重写。
- 能看懂并写出简单的例子：
  - `Base`/`Derived` 虚函数 + 基类指针容器 + `draw_all` 这种多态调用。


# 菱形继承

## 1. 什么是“菱形继承”以及为什么有问题？

经典结构：

```cpp
struct Base {
    int value;
};

struct Left : Base {};
struct Right : Base {};

// 菱形
struct Diamond : Left, Right {};
```

继承关系画成图长这样：

```
    Base
   /    \
 Left  Right
   \    /
  Diamond
```

`Diamond` 通过两条路径继承了 `Base`：

- `Diamond -> Left -> Base`
- `Diamond -> Right -> Base`

**结果：`Diamond` 里有两份 `Base` 子对象**。

带来的问题：

1. **重复基类子对象**  
   - `sizeof(Diamond)` 至少包含两份 `Base`。
2. **二义性**  

   ```cpp
   Diamond d;
   d.value = 42; // 编译错误：不知道是 Left::Base 还是 Right::Base 的 value
   ```

   你必须写成：

   ```cpp
   d.Left::value = 1;
   d.Right::value = 2;
   ```

   这很丑，而且很多时候你本意就是“我只想有一份 Base”。

---

## 2. 虚继承怎么解决这个问题？

把 `Left` / `Right` 改成**虚继承**：

```cpp
struct Base {
    int value;
};

struct Left : virtual Base {};
struct Right : virtual Base {};

struct Diamond : Left, Right {};
```

语义变成：

> “Left 和 Right 都是**虚继承** Base，遇到多条路径指向同一个虚基类时，**所有路径共享一份 Base 子对象**。”

也就是说：

- `Diamond` 里只有**一份** `Base` 子对象；
- `d.value` 不再二义，可以直接访问；
- 对象布局更类似：

  ```text
  [Diamond 自身数据]
  [Left 子对象，含指向虚基 Base 的间接信息]
  [Right 子对象，含指向虚基 Base 的间接信息]
  [共享的 Base 子对象（虚基）]
  ```

（具体布局由编译器决定，但可以这么理解。）

这样你就能写：

```cpp
Diamond d;
d.value = 42;           // OK，只有一份 Base
Left& l = d;
Right& r = d;
```

从 `Left` / `Right` 视角看到的 `Base` 子对象其实是同一份。

---

## 3. 虚继承的成本在哪里？

你 day6 里那句“让对象布局和指针调整更复杂”，主要指的是：

1. **对象布局更复杂**  
   - 普通单继承：对象内存布局基本是线性的，“Base 部分 + Derived 部分”。
   - 虚继承：编译器需要在对象里存储一些额外信息（类似“虚基表指针”、“偏移量”等），来表示“共享的虚基在哪儿”。  
   - 这会让对象体积增大、布局不再是一个简单的 `Base`+`Derived` 拼接。

2. **指针调整更复杂**  
   - 普通继承中，`Derived*` 转成 `Base*` 通常是一个固定偏移（甚至可以是编译期常量折叠为简单加法）。
   - 虚继承下，`Left*` / `Right*` 转成虚基 `Base*`，可能需要：
     - 通过某个“虚基表”查询偏移；
     - 再在运行时做一次指针调整。  
   - 也就是说，**从派生类指针/引用到虚基类指针的转换，可能需要一次间接查表**，不是简单的 `+offset`。

3. **实现/调试复杂度**  
   - 对你自己来说：  
     - debug 时看对象内存、用 `reinterpret_cast` 等调试手段会更难懂；
     - 设计层面，如果滥用虚继承，会迅速形成复杂的层次，很难维护。
   - 对编译器/ABI 来说：  
     - 虚继承影响布局和二进制兼容性，库升级、跨模块链接时需要非常小心。

性能上：

- 绝大多数情况下，虚继承这点额外成本不会成为瓶颈（比 IO/锁/缓存 miss 小太多）。
- 但它确实比普通单继承**多了一些指针间接访问和额外存储**，所以只在**确实需要共享虚基**时用，而不是随手乱加 `virtual`。

---

## 4. 你需要掌握到什么程度？

对菱形继承 + 虚继承，建议你达到下面这个水平：

- 能画出两个版本的图：
  - 非虚继承：`Diamond` 有两份 `Base`，访问 `d.value` 二义。
  - 虚继承：路径共享一份虚基 `Base`，`d.value` 唯一。
- 能用自己的话解释：
  - “虚继承解决了多路径继承下的重复基类问题，但让对象布局和指针调整更复杂，有一定运行时成本。”
- 能手写一个小例子（你 day6 的 `Base/Left/Right/Diamond`），并打印：
  - `sizeof(Base/Left/Right/Diamond)`；
  - 几个指针互转、调用虚函数，观察行为。

# 多继承到底解决什么问题

这个问题问得很好，现实里“菱形多继承”确实很少见，用不好还很恶心，所以要分开看：

- 多继承本身：**什么时候需要一个类“继承多个角色”**。
- 菱形 + 虚继承：**当这几个角色有共同基类时，怎么避免重复那一份基类**。

---

## 1. 多继承到底解决什么问题？

一句话：

> 当一个类型**同时扮演多个正交的角色/接口**时，多继承可以把这些角色拆成独立的基类，然后在一个派生类里“合并”。

常见几种用法：

### 1.1 “接口 + 接口” 型（更像 Java 的多接口）

例如：

```cpp
struct Drawable {
    virtual void draw() const = 0;
    virtual ~Drawable() = default;
};

struct Serializable {
    virtual std::string serialize() const = 0;
    virtual ~Serializable() = default;
};

struct Widget : Drawable, Serializable {
    void draw() const override { /* ... */ }
    std::string serialize() const override { /* ... */ }
};
```

- `Widget` 同时是一个 `Drawable`，也是一个 `Serializable`。
- 它可以被放到 `std::vector<Drawable*>` 里统一 `draw()`，  
  也可以被用在一套 `Serializable*` 接口里统一 `serialize()`。

这类多继承**不共享数据基类**，只是“多接口”，基本没有菱形问题，常见在：

- GUI 库、插件系统、事件系统；
- 某些库的“Tag 接口”或“能力接口”（capability interfaces）。

### 1.2 “接口 + 实现基类”混合

比如：

```cpp
struct RefCounted {
    void add_ref();
    void release();
protected:
    int ref_count_ = 0;
};

struct Drawable {
    virtual void draw() const = 0;
    virtual ~Drawable() = default;
};

struct Sprite : Drawable, RefCounted {
    void draw() const override { /* ... */ }
};
```

- `RefCounted` 提供一套通用的引用计数实现；
- `Drawable` 提供绘制接口；
- `Sprite` 同时复用实现 + 实现接口。

这种就已经带一点“实现多继承”的味道，要小心结构复杂化。

---

## 2. 菱形继承 + 虚继承的**特定意义**

再回到你刚写的形状（或者 Day6 里的例子）：

```cpp
struct Base { ... };
struct Left : virtual Base {};
struct Right : virtual Base {};
struct Diamond : Left, Right {};
```

真正有意义的场景是：

- `Left` 和 `Right` 都**需要**从 `Base` 继承一些共同状态/行为；
- 而 `Diamond` 需要**同时具备 Left 和 Right 的能力**；
- 但你又**不希望 `Diamond` 里出现两份 Base**（数据重复、语义二义）。

典型用法：

1. **多角色共享一套公共状态**  
   比如网络协议栈里：
   - `TcpConnection` 和 `TlsConnection` 都继承自 `ConnectionBase`（连接 ID、状态、日志等）；
   - 某个 `SecureTcpConnection` 既是 `TcpConnection` 又是 `TlsConnection`，  
     但你只想要**一份** `ConnectionBase` 状态，就会用虚继承。

2. **类层次拆分成“公共基 + 两个分支 + 汇合”**  
   有时架构演进出来的：  
   - 先有 `Base`；  
   - 后来拆出两个方向 `FeatureA : virtual Base`、`FeatureB : virtual Base`；  
   - 最后有个类型 `Both : FeatureA, FeatureB`，继承“两种特性”，同时共享一份 `Base`。

说白了：**虚继承 = 当你必须用菱形结构时，给你一个“只保留一份公共基类”的安全阀**。

---

## 3. 为什么工程实践里“用得少，还挺危险”？

现实项目里，多继承尤其是带菱形结构的，多半属于：

- **历史包袱 / 老框架**：早期设计激进，后来大家不敢动；
- 或者是某些**库作者的高阶技巧**，普通业务代码只是在“用”。

原因：

1. **可读性和认知负担高**  
   - 一般人看见菱形结构就头大；
   - 抽象一多，很难一眼看出“这个类到底有哪些状态，是从哪条链继承来的”。

2. **调试复杂**  
   - 内存布局复杂；
   - 指针强转、`dynamic_cast`、日志打印都更难懂。

3. **设计空间容易被滥用**  
   - 一旦大家发现“哦，多继承能这么玩”，就容易散落着一堆复杂层次；
   - 后期接手的人根本不敢动，只能堆新代码在旁边。

因此，**现代 C++ 社区的主流建议是**：

- 接口型多继承（纯虚接口）可以适度用；
- 有数据成员的多继承（尤其是菱形 + 虚继承）要非常谨慎，
  通常只在**必须共享公共基类状态**时才会考虑。

---

## 4. 对你来说，“知道到什么程度”才算够？

结合你现在的目标，我建议这样定位“有啥用”：

- 知道：  
  - **菱形继承本身会导致重复基类和二义性**；  
  - `virtual` 继承能把多条路径上的公共基类**合并为一个虚基对象**；
  - 代价是对象布局和指针调整更复杂，有一定性能和理解成本。
- 能手写并解释一个小例子：  
  - 非虚继承 vs 虚继承的 `Base/Left/Right/Diamond`；  
  - 打印 `sizeof`，演示 `d.value` 的二义性/唯一性。
- 面试时能说出一句**带态度的观点**：

  > 实际项目里我会非常克制地用带数据的多继承，菱形结构更是只在必须共享公共基类状态时才考虑，大多数场景会用组合或“接口多继承”来表达角色。

这就足够“懂原理，知道利弊，遇到相关代码不慌，也不会自己乱造菱形地狱”了。

# 例子

我用“订单 Order”来举例，对比三种写法：

- 上帝类（反面教材）
- 组合拆分小组件（推荐默认）
- 多继承 + mixin（展示优缺点）

代码我只贴核心结构，不追求可编译完整性。

---

## 1. 上帝类写法：所有逻辑全堆在一个类里（反例）

```cpp
class Order {
public:
    // 状态相关
    enum class Status { Created, Paid, Shipped, Cancelled };
    void pay();
    void ship();
    void cancel();

    // 日志相关
    void log(const std::string& msg);

    // 重试相关
    void retryPayment();

    // 统计相关
    void reportMetrics();

    // 序列化
    std::string to_json() const;
    static Order from_json(const std::string&);

private:
    // 业务状态
    std::string id_;
    Status status_;
    double amount_;

    // 重试状态
    int retry_count_;
    bool last_retry_success_;

    // 统计/监控
    uint64_t last_update_ts_;
    int metric_flag_;
};
```

- **优点**：
  - 一开始写起来快，所有东西“伸手可得”。
- **缺点**：
  - 职责极其混乱：状态机 + 日志 + 重试 + 监控 + 序列化全在一起。
  - 任何改动都会重新编译整个文件，耦合严重。
  - 单测难写：你想测重试逻辑，得 new 一个完整的 `Order`，状态、依赖一堆。
  - 很容易进一步演化成“什么都往里加”的真·上帝类。

---

## 2. 组合拆分：小类 + 大对象（推荐默认）

### 2.1 拆成小组件

```cpp
// 只管状态机
class OrderStateMachine {
public:
    enum class Status { Created, Paid, Shipped, Cancelled };

    explicit OrderStateMachine(Status s = Status::Created) : status_(s) {}

    void pay();
    void ship();
    void cancel();

    Status status() const { return status_; }

private:
    Status status_;
};

// 只管日志
class OrderLogger {
public:
    void log(const std::string& id, const std::string& msg) const {
        std::cout << "[Order " << id << "] " << msg << "\n";
    }
};

// 只管重试
class PaymentRetrier {
public:
    void retryPayment(const std::string& order_id, double amount);

private:
    int retry_count_ = 0;
};

// 只管监控/统计
class OrderMetrics {
public:
    void markUpdated(const std::string& id);
};
```

### 2.2 用组合装配成“大对象”

```cpp
class Order {
public:
    using Status = OrderStateMachine::Status;

    Order(std::string id, double amount)
        : id_(std::move(id)), amount_(amount) {}

    void pay() {
        logger_.log(id_, "pay");
        state_.pay();
        metrics_.markUpdated(id_);
    }

    void retryPay() {
        retrier_.retryPayment(id_, amount_);
        metrics_.markUpdated(id_);
    }

    Status status() const { return state_.status(); }

private:
    std::string id_;
    double amount_;
    OrderStateMachine state_;
    OrderLogger logger_;
    PaymentRetrier retrier_;
    OrderMetrics metrics_;
};
```

- **优点**：
  - 每个小类**单一职责**，容易理解、修改、单测：
    - 想测状态机，就只测 `OrderStateMachine`；
    - 想换一套日志实现，就改 `OrderLogger` 即可。
  - `Order` 成为一个“编排者 / 门面”，本身比较瘦。
  - 不涉及继承，结构直观，大部分人一看就懂。

- **缺点**：
  - `Order` 里有一些“转发胶水”（`logger_.log`、`retrier_.retryPayment` 等）；
  - 如果很多地方都需要日志/重试能力，可能会重复组合代码（不过这通常可接受）。

---

## 3. 多继承 + mixin：把“能力”做成可叠加的小基类

假设我们想做“有日志能力的”、“有重试能力的”mixin：

```cpp
class HasOrderLogging {
protected:
    void log(const std::string& id, const std::string& msg) const {
        std::cout << "[Order " << id << "] " << msg << "\n";
    }
};

class HasPaymentRetry {
protected:
    void retryPayment(const std::string& order_id, double amount) {
        std::cout << "retry payment for " << order_id
                  << ", amount=" << amount << "\n";
        // 内部可以有自己的状态
        ++retry_count_;
    }

private:
    int retry_count_ = 0;
};
```

然后 `Order` 用多继承叠加这些“能力”：

```cpp
class Order
    : private HasOrderLogging   // 多继承 mixin
    , private HasPaymentRetry
{
public:
    using Status = OrderStateMachine::Status;

    Order(std::string id, double amount)
        : id_(std::move(id)), amount_(amount) {}

    void pay() {
        log(id_, "pay");       // 直接使用 mixin 提供的接口
        state_.pay();
        metrics_.markUpdated(id_);
    }

    void retryPay() {
        retryPayment(id_, amount_);  // 直接用 mixin 的函数
        metrics_.markUpdated(id_);
    }

    Status status() const { return state_.status(); }

private:
    std::string id_;
    double amount_;
    OrderStateMachine state_;
    OrderMetrics metrics_;
};
```

- **优点**：
  - `Order` 的实现看起来“干净”：像是天然就有 `log`、`retryPayment` 这些成员。
  - 如果有别的类（比如 `Invoice`）也需要**完全相同**的日志/重试能力，可以直接继承这些 mixin：
    ```cpp
    class Invoice : private HasOrderLogging, private HasPaymentRetry { ... };
    ```
- **缺点 / 风险**：
  - 阅读成本高：`log` / `retryPayment` 是从哪来的，需要去看基类声明。
  - 一旦 mixin 内部有状态、且多个 mixin 之间有关系（依赖顺序、调用约束），理解起来迅速变难。
  - 如果 mixin 本身再做继承（比如多个 mixin 共享一个基类），很容易引出菱形/虚继承问题。
  - ABI / 二进制布局变复杂，对大型项目（多个模块链接）有隐藏成本。

---

## 4. 总结：在这个例子里的取舍建议

- **思想上**：你之前公司说的“用小类 + 多继承避免上帝类”是在**对抗上帝类**, 这是正确的方向。
- **落地上**，我会建议你优先这样排序：
  1. **优先组合**：像例子里的 `OrderStateMachine`、`OrderLogger` 等；
  2. **接口用多继承是 OK 的**：纯虚接口，没数据；
  3. **实现用多继承 + mixin 要谨慎**：  
     - 只在 mixin 职责非常单纯、状态简单时用；  
     - 严格避免复杂的层次关系，尤其是菱形结构和虚继承。

---

# OOP 设计原则与取舍

## 1. 组合 vs 继承：“能用组合就别用继承”到底在说啥？

### 1.1 直觉区别

- **继承（inheritance）**：  
  “`Derived` 是一个 `Base`”（`is-a` 关系）。  
  派生类自动获得基类接口和实现，耦合度高，类型层次固定。

- **组合（composition）**：  
  “`X` 里面**有一个** `Y`”（`has-a` 关系）。  
  `X` 通过成员对象使用 `Y` 的能力，可以随时替换掉这个成员，耦合度低。

### 1.2 为什么默认推荐组合？

因为继承有几个天然风险：

- **类型层次一旦定死，很难重构**：  
  某个基类改一点点，会影响所有派生类；
- **容易滥用**：  
  很多“代码复用”明明用组合就行，却搞成继承，结果派生类莫名其妙多了一堆不该暴露的接口；
- **多继承 / 虚继承带来复杂度**：  
  前面我们已经讲了一大堆。

而组合的优点：

- 更**局部**：只在成员上产生依赖；
- 更**易替换**：想换实现，只要换成员类型/通过接口注入新对象；
- 对阅读者很直观：“这个类里面依赖了哪些组件”，一看成员就知道。

**经验规则**：

- 如果你要表达“`Derived` 真的是一种 `Base`，并且需要多态” → 可以考虑继承；
- 如果只是“想复用点代码/功能”，但语义上不是 `is-a`，优先用组合。

---

## 2. 接口 / 实现分离：让依赖指向“抽象”而不是“细节”

这条其实是面向对象设计里非常核心的一条思想：

> 高层模块不应该依赖低层模块，两者都应该依赖抽象。  
> —— 也就是大家常说的“依赖倒置”。

在 C++ 里，最常见的几种做法：

### 2.1 面向接口编程

用一个纯虚基类（接口）来抽象能力，实际实现放在派生类：

```cpp
struct IStorage {
    virtual ~IStorage() = default;
    virtual void put(std::string key, std::string value) = 0;
    virtual std::string get(std::string key) = 0;
};

class RedisStorage : public IStorage { /* ... */ };
class FileStorage  : public IStorage { /* ... */ };

class SessionManager {
public:
    explicit SessionManager(std::unique_ptr<IStorage> storage)
        : storage_(std::move(storage)) {}

    // 业务代码只依赖 IStorage 抽象，不关心是 Redis 还是文件
private:
    std::unique_ptr<IStorage> storage_;
};
```

好处：

- `SessionManager` 依赖一个**稳定的、简单的接口**；
- 具体选择 `RedisStorage` 还是 `FileStorage`，可以在别的地方决定（配置/工厂），测试里也可以塞一个 `MockStorage`。

这是“接口/实现分离”的典型模式：  
**调用方只看到接口，具体实现可以自由替换。**

### 2.2 接口与数据结构分离

有时我们还可以把“对外 API”单独放一个头文件，真正的实现/数据结构放 cpp 里：

- 头文件只暴露函数签名 & 少量类型；
- 实现细节藏在 .cpp 里，减少编译依赖，降低改动波及范围。

pimpl 就是这种思想的加强版。

---

## 3. pimpl（pointer to implementation）：隐藏实现、降低编译依赖

### 3.1 pimpl 是什么？

标准写法长这样：

```cpp
// Foo.h
class Foo {
public:
    Foo();
    ~Foo();
    Foo(Foo&&) noexcept;
    Foo& operator=(Foo&&) noexcept;

    void do_something();

private:
    struct Impl;            // 前置声明
    std::unique_ptr<Impl> impl_;  // 指向真正实现的指针
};
```

```cpp
// Foo.cpp
struct Foo::Impl {
    int a;
    std::string s;
    std::vector<int> data;

    void do_something_impl() {
        // 具体逻辑...
    }
};

Foo::Foo() : impl_(new Impl) {}
Foo::~Foo() = default;

Foo::Foo(Foo&&) noexcept = default;
Foo& Foo::operator=(Foo&&) noexcept = default;

void Foo::do_something() {
    impl_->do_something_impl();
}
```

- 头文件 `Foo.h` 里：
  - `Foo` 的大小 = 一个指针的大小（或一个智能指针）；
  - **看不到** `Impl` 里有哪些成员、依赖了哪个头文件；
- 实现细节 (`Impl`) 完全在 `Foo.cpp` 里，可以随便 `#include <vector>`、`#include "BigLib.h"` 等。

### 3.2 pimpl 有什么用？

1. **隐藏实现细节（信息隐藏）**  
   - 使用者只能看到 `Foo` 的接口，不知道内部用了什么容器、第三方库；
   - 有利于封装和 API 演进。

2. **降低编译依赖 & 加快编译**  
   - 谁 `#include "Foo.h"`，只会看到一个 `unique_ptr<Impl>` 和一些函数声明；
   - 如果你修改 `Impl` 的成员，只需要重新编译 `Foo.cpp`，不需要重编所有包含 `Foo.h` 的文件；
   - 在大型项目里，这对编译时间提升很明显。

3. **二进制兼容（ABI stability）**  
   - 在需要维护 ABI 稳定的库（比如发行 SDK）里，pimpl 非常常见：
     - 头文件里的类大小不变（一个指针）；
     - 可以在不破坏 ABI 的前提下，在 Impl 里加字段、换容器。

### 3.3 pimpl 的代价是什么？

你 day6 里那句话“多一层 indirection（间接访问）”，就是指：

- 每次访问内部成员，都要通过 `impl_->xxx`，多一次指针解引用；
- 内存布局变成“对象里有一个指向真正数据的堆对象”的模式，多一次堆分配（除非用自定义 alloc 或 small buffer 优化）；
- 写代码时要多写一堆“转发”和“特殊的五大函数（构造/析构/移动）”。

简单讲：

> 你用 **一点运行时开销 + 一点编码复杂度**，换来 **更好的封装和编译时间/ABI 稳定**。

绝大部分业务场景，这点开销完全可以接受；  
超高性能/超热点路径里，就要仔细评估是否值得。

---

## 4. 三者怎么组合在一起用？

你可以把这三条设计原则串成一条思路（也方便面试时讲）：

1. **组合 vs 继承**  
   - 优先用组合把一个大概念拆成多个小组件（状态机、日志、重试、监控）；
   - 只有当语义上真的是 `is-a` 且需要多态时，才用继承。

2. **接口 / 实现分离**  
   - 对外暴露稳定的抽象接口（纯虚接口或非虚 API）；  
   - 具体实现通过组合/多态藏在后面，调用方只依赖接口。

3. **pimpl**  
   - 当类的内部实现经常变，或依赖非常重的头文件时，用 pimpl 把实现藏进 cpp；
   - 这样你既保持了对外接口的稳定，又不会因为内部一点改动就引起大面积重编。

# pimpl 再讲解
我换一个更“工程+比喻”的方式，把 pimpl 讲清楚。

---

## 0. 一句话版本先给你

> **pimpl = 把类的“内脏”搬到 .cpp 里，用一个指针指过去**。  
> 目的是：**头文件只暴露稳定的“外壳”，内部实现爱怎么变就怎么变**，不拖累所有 include 这个头的代码重新编译。

---

## 1. 不用 pimpl 时，问题出在哪？

假设你这么写：

```cpp
// Foo.h
#include <vector>
#include <string>

class Foo {
public:
    void do_something();

private:
    std::vector<int> data_;
    std::string name_;
};
```

```cpp
// Foo.cpp
#include "Foo.h"

void Foo::do_something() {
    // ... 用 data_ / name_ 做一堆事 ...
}
```

问题：

1. **所有包含 `Foo.h` 的地方，都能看到实现细节**：
   - 知道你用了 `std::vector<int>` 和 `std::string`；
   - 想改容器类型（比如换成 `std::deque`）、多加几个成员字段，就会改动头文件。

2. **一改实现字段，所有 include Foo.h 的文件都要重编**：
   - 比如你给 `Foo` 多加一个 `std::map` 成员；
   - 上百个源文件只要 `#include "Foo.h"`，都要被重新编译一次。

在小项目里没感觉，大项目里这会严重拖慢编译。

---

## 2. pimpl 做的事：把“内脏”塞进 Impl

同一个类，用 pimpl 写长这样：

```cpp
// Foo.h
#include <memory>  // 头文件只需要知道智能指针

class Foo {
public:
    Foo();
    ~Foo();
    Foo(Foo&&) noexcept;
    Foo& operator=(Foo&&) noexcept;

    void do_something();

private:
    struct Impl;                  // 只声明，不定义
    std::unique_ptr<Impl> impl_;  // 指向真正实现对象
};
```

```cpp
// Foo.cpp
#include "Foo.h"
#include <vector>
#include <string>

struct Foo::Impl {
    std::vector<int> data_;
    std::string name_;

    void do_something_impl() {
        // ... 真正实现 ...
    }
};

Foo::Foo() : impl_(new Impl) {}
Foo::~Foo() = default;

Foo::Foo(Foo&&) noexcept = default;
Foo& Foo::operator=(Foo&&) noexcept = default;

void Foo::do_something() {
    impl_->do_something_impl();
}
```

**关键对比：**

- 头文件 `Foo.h` 现在：
  - 不再 `#include <vector>` `<string>`；
  - 外面只看到一个 `std::unique_ptr<Impl>`，不知道 Impl 里有什么。
- 所有实现细节（字段、容器、算法）都藏在 `Foo.cpp` 里的 `Foo::Impl` 中。

---

## 3. 这样做到底换来了什么？

### 3.1 封装更彻底

- 调用方**看不到你的内部成员类型**，也就不能依赖它们：
  - 不能直接访问 `data_`；
  - 不会因为你把 `std::vector` 换成 `std::deque` 而需要改任何调用处。

这是一种“强封装”：  
你逼着别人只能通过 `Foo` 的接口来和你交互。

### 3.2 编译依赖小很多

- 谁 `#include "Foo.h"`：
  - 只会看到一个指针、几个函数声明；
  - `Impl` 的定义和所有大头文件只在 `Foo.cpp` 里 include。

- 你在 `Foo::Impl` 里怎么折腾（加字段/换容器/多 include 一堆别的库），  
  **只需要重编译 Foo.cpp**，  
  不用重编所有 include Foo.h 的文件。

在大项目、SDK、底层库里，pimpl 很常见就是因为这一点。

### 3.3 ABI 稳定（如果你发库给别人）

假设你给外部发了一个二进制库（.so/.dll），外面的人只拿到头文件 `Foo.h`：

- 传统写法：类里成员一变，**类大小、布局都变了**，ABI 就破了，外部重新链接有风险；
- pimpl 写法：  
  - 头文件里的 `class Foo` 大小就是一个指针的大小，不变；  
  - 你可以在 Impl 里随便加成员字段，外部二进制布局还是兼容的。

如果将来你要写 C++ SDK，这点会非常有用。

---

## 4. 代价：你付出了什么？

主要是两块负担：

1. **运行时一点开销**：
   - 每次访问内部数据，都要经由 `impl_` 指针：
     - 多一次指针解引用；
     - 默认实现里通常有一次堆分配（`new Impl`）。
   - 大多数业务代码这点 overhead 完全无感；
   - 极端高性能/内存紧张路径要谨慎用。

2. **编码复杂度**：
   - 要写 `Impl` 结构体；
   - 要转发一堆接口（`do_something()` 调 `impl_->do_something_impl()`）；
   - 要好好写/默认生成构造、析构、移动等，避免资源泄漏。

你可以把 pimpl 理解成：

> 拿一点运行时成本 + 一点写代码麻烦，换更好的封装、编译时间和二进制稳定性。

---

## 5. 什么时候适合你用 pimpl？

我帮你定个简单决策表：

- **适合用 pimpl 的场景**：
  - 这是一个**对外暴露的核心类**（模块/库/SDK 的 public API）；
  - 内部很复杂，依赖很多重头文件（三方库、庞大 STL 容器）；
  - 预计未来实现细节会经常改动，但想让**对外接口尽量稳定**。

- **不太值得用 pimpl 的场景**：
  - 只是某个业务模块的内部小类；
  - 不太会被很多文件 include；
  - 内部结构也不太会频繁变化。

> pimpl = “把类的私有实现用 `Impl` + 指针藏进 cpp，只在头文件露出一层薄壳”。

# 容器复杂度与迭代器失效

## 1. 五大容器的典型复杂度（大致直觉就够）

粗略记一个“考试用版本”：

- **`vector`**
  - 随机访问 `v[i]`：**O(1)**。
  - 尾部插入/删除：摊还 **O(1)**（偶尔扩容变成 O(n)）。
  - 中间插入/删除：**O(n)**（要搬动后面的元素）。
- **`list`（双向链表）**
  - 任意位置插入/删除（已有迭代器处）：**O(1)**。
  - 按值查找：**O(n)**。
  - 不支持随机访问（`it + 5` 不行，`operator[]` 没有）。
- **`deque`**
  - 头尾插入/删除：**O(1)** 摊还。
  - 中间插入/删除：**O(n)**。
  - 随机访问 `d[i]`：**O(1)**。
- **`map`（有序平衡树）**
  - 按 key 插入/查找/删除：**O(log n)**。
  - 中序遍历有序。
- **`unordered_map`（哈希表）**
  - 按 key 插入/查找/删除：平均 **O(1)**，最坏 O(n)。
  - 遍历无序，迭代顺序不稳定。

记住这个“直觉表”就足够应付大部分面试题。

---

## 2. 迭代器失效：各容器关键规则

### 2.1 `vector`

核心记忆：

> “**可能触发重分配的操作，会把所有迭代器/指针/引用都干掉**；  
> 只是改变大小但不重分配的操作，会保持已有元素的引用/指针有效。”

- 会导致 **所有** 迭代器/指针/引用失效 的操作：
  - `push_back` / `emplace_back`，当容量不足发生 **realloc** 时；
  - `insert`、`emplace` 插入新元素，如果触发重分配；
  - `reserve` 减少不了失效，**扩大容量时也会重分配**，也会让原来的迭代器/指针失效；
  - `shrink_to_fit`（实现可以选择重分配）。
- 即使**不重分配**，也会让从插入位置开始的**后面那一段**迭代器失效的操作：
  - `insert` / `erase` 中间位置：
    - 前面的元素引用/指针还有效；
    - 被删除/被移动的那一段的迭代器/指针失效。

简单背法：

- “**realloc → 全灭**；中间插/删 → 插/删点后的失效。”
- `resize` 可能触发 realloc（等价于 push_back 一堆），所以也按上面看。

### 2.2 `list`

> list 是“**指针稳定容器**”：不会整体搬家。

- 插入/删除节点不会让**其他节点**的迭代器/指针/引用失效；
- 只有指向被删除那个节点的迭代器/指针失效；
- 插入新节点前后会新产生迭代器，但旧的仍有效。

所以对 list，迭代器失效基本只和“这个元素是否被删掉”有关。

### 2.3 `deque`

可以记作“**中间插删、容量变化，很多情况都不安全**”，面试时简单说：

- 在头/尾插入或删除：
  - 有可能导致部分迭代器失效（特别是插入导致新块分配时）；
  - 标准并不保证像 list 一样稳定，**保守地认为：头/尾插删会使迭代器失效**。
- 中间插入/删除：
  - 可能移动很多块，普遍视为“相关迭代器失效”。

实务上，如果面试没要求背特别细，可以说这一句：

> 对 `deque` 我一般避免长期持有迭代器/指针，特别是在做插入/删除之后，会重新获取迭代器。

### 2.4 `map` / `set`（有序树）

- 插入元素：
  - **不会让已有元素的迭代器失效**；
  - 迭代器稳定。
- 删除元素：
  - 只让指向该元素的迭代器/引用失效；
  - 其他元素的迭代器仍然有效。

可以简单记成：

> “`map` / `set` 插入安全，删除只杀自己。”

### 2.5 `unordered_map` / `unordered_set`（哈希表）

这里要小心：

- 当 **触发 rehash (扩容/换桶)** 时：
  - 所有迭代器失效；
  - 对于 `unordered_map`，指针/引用指向的元素本身仍然存在（节点通常在堆上），但标准只保证引用有效，不保证迭代器稳定。
- 插入元素但 **不 rehash** 时：
  - `unordered_map::insert` / `emplace`：
    - 标准：可能使迭代器失效（实现相关），保守起见认为有风险。
- 删除元素：
  - 只让该元素的迭代器/引用失效。

稳妥的说法：

> `unordered_map` 在 rehash 时会使所有迭代器失效，删除使对应元素的迭代器失效，引用/指针一般指向节点本身，多数实现不会移动节点内存，但不要指望遍历顺序不变。

---

## 3. `reserve` 和 `emplace`：对迭代器和性能的影响

### 3.1 `reserve` 对 `vector` 的影响

`reserve(n)` 只影响容量，不改变 `size()`，可能会触发一次 **重分配**：

- 如果 `n > capacity()`：
  - 会分配新空间，把原元素搬过去；
  - 所有旧的迭代器/指针/引用失效；
  - 之后再 `push_back` 不会再频繁重分配 → **性能更好**、**避免多次失效**。
- 如果 `n <= capacity()`：
  - 什么也不做，不改变现有迭代器。

你的直觉可以是：

> “`reserve` 就是**一次性把所有该付的搬家成本交了**，以后再慢慢住进去不再搬。”

性能好处：

- 避免 `push_back` 时一遍遍扩容搬元素；
- 对有大量插入（尤其是尾插）场景明显。

迭代器安全性：

- 在**调用 `reserve` 那一刻**，如果发生扩容，**所有旧迭代器失效**，之后要重新取迭代器。

### 3.2 `emplace` vs `push`：性能 & 失效

以 `vector` 为例：

```cpp
v.push_back(T{x, y});      // 构造一个临时 T，然后拷贝/移动进容器
v.emplace_back(x, y);      // 在容器内存上原地构造 T
```

- 性能方面：
  - `emplace_back` **可以省掉一次临时对象**，尤其是不可移动/拷贝开销大的对象；
  - 对简单类型（int、string 小对象）差别不大。
- 迭代器失效规则方面：
  - 对 `vector` 而言，`push_back` 和 `emplace_back` 在“是否触发扩容”这件事上没有本质差别：
    - 一旦扩容，**两者都会让迭代器/引用失效**；
    - 没扩容，两者都只是尾部添加一个新元素，不影响原有迭代器。

简单记：

> `emplace` 主要是**构造方式的性能优化**，  
> 对迭代器失效规则几乎和对应的 `push` 一样看待。

对 `map` / `unordered_map` 等：

- `insert` 接受现成 `std::pair`；
- `emplace` 接受构造 pair 的参数，在容器内部直接构造，减少一次 pair 的构造/移动；
- 迭代器失效规则依旧按容器本身的规则来（比如 `unordered_map` rehash 时统一失效）。

---

## 4. 给你一个“考试速记版”小结

可以直接抄到 [docs/day6_learn.md](cci:7://file:///home/lisa/cpp_master/docs/day6_learn.md:0:0-0:0) 末尾当 checklist：

- **vector**
  - 复杂度：随机访问 O(1)，尾插均摊 O(1)，中间插删 O(n)。
  - 失效：扩容（push_back / insert / reserve）→ 所有迭代器/指针/引用失效；中间插删→ 插删点之后的失效。
- **list**
  - 任意位置插删（给定迭代器）O(1)，迭代器稳定；
  - 只删除的那个元素的迭代器失效，其它都有效。
- **deque**
  - 头尾插删 O(1) 摊还，中间 O(n)；
  - 插删后迭代器不稳定，保守认为“修改后重新拿迭代器”。
- **map/set**
  - 插入/删除/查找 O(log n)；
  - 插入不会让已有迭代器失效，删除只杀自己。
- **unordered_map/set**
  - 平均 O(1) 查找/插入/删除；
  - rehash 时所有迭代器失效，删除只杀自己。

- `reserve`：
  - 为 vector 提前扩容，减少多次重分配；  
  - 扩容那一刻会让旧迭代器/指针/引用全部失效，以后 push_back 更平滑。
- `emplace`：
  - 减少不必要的临时对象/拷贝；  
  - 迭代器失效规则和对应的 `push` 基本一致（主要看容器是否扩容/rehash）。

---

# 哈希表和红黑树

## 1. 哈希表（`unordered_map` 等）

### 1.1 核心思想

> 用 **哈希函数** 把 key 映射到一个“桶下标”，  
> 把元素按桶存起来，查找时只在对应桶里找，而不是全表扫。

结构大致是：

- 一个 **bucket 数组**（vector-like）；
- 每个 bucket 里是一条链表或小数组（链式哈希、开放寻址等实现差异）。

### 1.2 复杂度直觉

在“平均情况下”（哈希函数好、负载因子合适）：

- 查找：**O(1) 平均**（只看一个桶的少量元素）。
- 插入：**O(1) 平均**（可能偶尔 rehash 成 O(n)）。
- 删除：**O(1) 平均**。

最坏情况：哈希函数退化（所有 key 都进一个桶）→ 退化成 **O(n)**。

### 1.3 rehash / 负载因子

- **负载因子**（load factor）：`元素个数 / bucket 数量`。
- 当负载因子超过一定阈值（比如 1.0，具体实现不同）：
  - 容器会做 **rehash**（扩容 + 重新分配桶）；
  - 所有元素重新分桶，所有迭代器失效。

面试可以这样说：

> 哈希表为了维持 O(1) 平均复杂度，会在元素过多时扩容并 rehash；  
> 这一过程是 O(n) 的，但摊到每次插入上仍然是均摊 O(1)。

### 1.4 常见问法和要点

- **Q：为什么哈希表查找是 O(1) 平均？**  
  A：因为 key 通过哈希函数定位到桶，只在这个桶里（期望）常数个元素里查找，而不是线性扫整个容器。

- **Q：为什么遍历顺序不稳定？**  
  A：跟桶分布相关，rehash 后桶数变了，元素在数组中的位置完全不同；  
  所以不要依赖 `unordered_map` 的遍历顺序，甚至不同运行/不同版本也可能不同。

- **Q：什么时候会迭代器失效？**  
  A：
  - rehash 时：所有迭代器失效；
  - 删除元素：指向该元素的迭代器失效；
  - 引用/指针通常仍指向同一块节点内存（多数实现），但**标准只严格保证引用到元素本身有效，遍历顺序不保证**。

- **Q：和 `map` 相比有什么 trade-off？**  
  A（简洁版）：
  - `unordered_map`：平均 O(1) 查找，无序，适合频繁按 key 查；
  - `map`：O(log n) 查找，有序，适合需要按 key 排序/范围遍历的场景。

---

## 2. 红黑树（`map` / `set` 背后）

### 2.1 是什么？

> 红黑树 = 一种带颜色标记的自平衡二叉搜索树（BST）。

目标：保证树的高度始终是 **O(log n)**，避免退化成链表。

**二叉搜索树性质**：

- 对任意节点 `x`：
  - 左子树所有 key < x.key；
  - 右子树所有 key > x.key。

在此基础上加上红黑规则，保证大致平衡。

### 2.2 红黑树的关键性质（面试版）

常见说法是 5 条性质，记住大意就好：

1. 每个节点要么红，要么黑。
2. 根是黑色。
3. 所有叶子（NIL 空节点）是黑色（实现细节）。
4. 红色节点的子节点必须是黑色（**不能有连续两个红色节点**）。
5. 从任意节点到其所有叶子的路径上，黑色节点数目相同（**黑高相等**）。

推论：  
树的高度 ≤ 2 * log₂(n+1)，即高度是 O(log n)，不会退化得太离谱。

所以：

- 插入/删除时通过若干次旋转+变色，把树重新恢复到满足这些性质；
- 每次调整的成本是 **O(log n)**。

### 2.3 `std::map` / `std::set` 的特性

基于红黑树（或等价的平衡 BST，如 AVL）：

- 查找、插入、删除：**O(log n)**。
- 迭代器遍历是 **按 key 有序** 的（中序遍历）。
- 迭代器非常稳定：
  - 插入：不会使已有元素迭代器失效；
  - 删除：只让被删元素的迭代器失效。

这个有序性，直接解锁了一堆常用操作：

- 范围查询：`lower_bound` / `upper_bound`。
- 按区间遍历：`for (it = map.lower_bound(l); it != map.upper_bound(r); ++it)`。

### 2.4 常见问法和要点

- **Q：`map` 的查找复杂度？为什么？**  
  A：O(log n)，因为底层是自平衡 BST（通常是红黑树），高度 O(log n)，从根往下比较 key 走一条路径。

- **Q：为什么 `map` 插入不会让已有迭代器失效？**  
  A：插入只是改变树指针结构，节点分配在堆上，不移动已有节点；  
  所以指向其他节点的迭代器仍然有效。

- **Q：什么时候要用 `map` 而不是 `unordered_map`？**  
  - 需要 **有序遍历**（比如按 key 排序输出、按范围 [l, r] 查询）；
  - 需要 `lower_bound` / `upper_bound` / `equal_range` 之类的顺序操作；
  - 对 hash 不敏感、或者 key 本身没有合适的 hash（比如自定义复杂类型，比较运算比 hash 好写）。

- **Q：红黑树和 AVL 树区别？为啥 STL 通常选红黑树？**  
  简略说法：
  - AVL 更“严格平衡”，查找稍微好一点，但插入/删除重平衡成本更高；
  - 红黑树平衡要求略宽松，但插入/删除重平衡旋转次数更少；
  - 实际工程里红黑树综合性能更好，也更容易实现，所以被广泛采用。

---

## 3. 面试上可以怎么“背诵总结”

- **哈希表（unordered_map）**
  - 结构：bucket + 链表/数组，通过 hash(key) 找桶，平均 O(1) 查找。
  - 特点：无序，遍历顺序不稳定；负载因子过高会 rehash，导致所有迭代器失效。
  - 适用：频繁按 key 查，只关心 key→value 映射，不关心顺序。

- **红黑树（map/set）**
  - 结构：自平衡 BST，满足红黑性质，保证高度 O(log n)。
  - 特点：有序遍历，支持 `lower_bound` 等范围查询；插入不使迭代器失效，删除只杀自己。
  - 适用：需要按 key 排序、范围查询、有序遍历的场景。

---

# 哈希冲突

我帮你把“哈希冲突”这一块整理成几个常见考点：

- 概念：什么是冲突，为什么不可避免  
- 常见解决策略：开链法 / 开放寻址  
- 冲突对性能的影响  
- 工程里能做/不能做什么（load factor、定制 hash、DoS 之类）

---

## 1. 什么是哈希冲突，为什么一定会有？

- 哈希函数：`hash(key) -> 一个整数`，再对 bucket 数量取模当下标。
- 冲突（collision）：**不同的 key 经过哈希后落在同一个桶**。

为什么不可避免？

> key 的空间一般是“无限大”或极大（比如所有字符串），bucket 数是有限的。  
> 鸽笼原理：**不同 key 映射到同一个下标是必然发生的**，只能“控制概率和分布”，不能消灭。

---

## 2. 常见冲突解决方式

C++ 标准库实现细节不固定，但你要知道两大类思路：

### 2.1 开链法（separate chaining）

典型结构：

```text
bucket[0] -> list: [ (k0,v0) -> (k1,v1) -> ... ]
bucket[1] -> list: ...
...
```

- 每个 bucket 里挂一条链表（或小 vector），存所有 hash 到这个桶的元素。
- 查找：
  - 先算桶号，定位到那条链；
  - 再在链表里线性查找。

特点：

- 冲突发生时，只是链表变长；
- rehash（扩桶）时，要把所有元素重新挂到新桶数组对应位置；
- 在 std::unordered_map 中很常见（GCC libstdc++ 走桶 + 链表这种结构）。

### 2.2 开放寻址（open addressing）

思想：**所有元素都存在同一个数组里，没有外部链表**。

插入时：

- 如果 `bucket[i]` 已经被占，就按照某种探查序列找下一个空位：
  - 线性探查：`i+1, i+2, ...`；
  - 二次探查：`i+1², i+2², ...`；
  - 双重哈希：`i + k * hash2(key)`。

查找时也按同样的探查序列走，直到找到 key 或遇到空位。

特点：

- cache 友好（元素紧挨着在一块大数组里）；
- 删除比较麻烦（要标记“墓碑”防止断开探查链）；
- 对负载因子更敏感：装太满会极大恶化性能。

标准库实现可以是开链，也可以是开放寻址，这取决于具体库；  
你面试时只要知道这两类思想，就足够。

---

## 3. 冲突对性能的影响

**平均情况**（hash 分布均匀，负载因子合理）：

- 一个桶里的元素数期望是常数；
- 查找/插入/删除都是 O(1) 平均。

**冲突多了会怎样？**

- 开链：某些桶的链特别长，查找退化到 O(k)，k 是冲突个数；
- 开放寻址：探查次数增多，可能一直跳来跳去找空位，也变慢。

极端最坏情况（有人恶意构造一堆 key 让 hash 全落一个桶）：

- 所有操作退化成 O(n)，哈希表性能和链表没区别；
- 这也是某些安全场景（web 服务器）会提到哈希 DoS 攻击的原因：  
  攻击者构造大量冲突 key，让服务器在处理 hash 结构时非常慢。

---

## 4. 工程里我们能做什么来减轻冲突？

几条常见手段（你可以记在脑子里，写代码时顺手就做对）：

- **控制负载因子**：
  - 对 `unordered_map` 可以用 `reserve` 预留合理容量，减少 rehash 次数；
  - 如果你发现一个桶总是装太多元素，说明 load factor 太高或哈希函数不好。

- **选一个“还行”的哈希函数**：
  - 对内建类型，标准库的 `std::hash` 一般够用；
  - 对自定义类型，实现 `std::hash<T>` 或自定义哈希时：
    - 尽量让相似 key 也能分散开（不要只 hash 某个极易重复的字段）；
    - 避免简单拼接（如 `a * 131 + b` 太天真也可能被针对）。

- **不要依赖 unordered 的迭代顺序**：
  - 不要写“依赖当前遍历顺序”的逻辑，否则一旦 rehash/实现换了行为会炸；
  - 只把它当做 key→value 映射结构。

- **安全相关的注意点**（知道即可，一般不用自己实现）：
  - 一些语言/库会针对 `unordered_map` 提供“随机化的 hash 种子”，  
    减少被恶意构造冲突的概率；
  - C++ 标准库有没有这类防御是实现相关的，通常不要求你背。

---

## 5. 面试可能会怎么问“哈希冲突”？

你可以准备这样的回答模版：

> 哈希表不可避免会有冲突，不同 key 可能 hash 到同一个桶。  
> 常见的冲突解决策略有开链法和开放寻址法：  
> 开链是每个桶挂一条链表或小数组，冲突的元素放到同一个桶链里；  
> 开放寻址是所有元素存在一个数组里，如果某个桶被占，就按探查序列找下一个空位。  
> 在平均 hash 分布良好且负载因子适中的情况下，查找插入删除都是 O(1) 平均；  
> 但如果冲突很多，比如所有 key 都落在一个桶里，就会退化到 O(n)。  
> 工程里一般通过合理的 hash 函数、控制负载因子（比如提前 reserve），  
> 来降低冲突概率和 rehash 频率。

# 什么情况下需要rehash

这里面试常问的，其实是在考你对 `unordered_map` 的直觉：**什么时候需要把所有元素重新打散到新的桶数组里**。

我按“标准规则 + 常见触发点 + 不会触发的情况 + 面试说法”来讲。

---

## 1. 标准规则：load factor 超过阈值时会 rehash

对 `std::unordered_map` / `unordered_set`：

- 有两个关键量：
  - `bucket_count()`：当前桶个数；
  - `load_factor()`：当前负载因子 = `size() / bucket_count()`；
  - `max_load_factor()`：允许的最大负载因子（默认大约是 1.0，具体实现略有不同）。
- **当插入新元素后 `load_factor() > max_load_factor()` 时，容器会自动增加桶数并 rehash**：
  - 分配新的 bucket 数组；
  - 把所有已有元素重新计算桶号，挂到新的 bucket 上；
  - 所有迭代器失效。

可以理解为：

> “装得太满了、冲突太多了，就要换一个更大的桶数组，把所有元素重新打散一次。”

---

## 2. 哪些操作会触发 rehash？

### 2.1 自动触发（最常见）

- `insert` / `emplace` / `operator[]` 等“**增加元素数量**”的操作：
  - 如果插入后 `size()` 超过 `bucket_count() * max_load_factor()`，就会 rehash。
- 批量插入（range insert）本质上也是这个逻辑。

### 2.2 手动触发

标准还提供两种“你自己强制 rehash”的方式：

1. `reserve(n)`：

   ```cpp
   umap.reserve(1000);
   ```

   - 含义：希望能容纳至少 `n` 个元素而不再 rehash；
   - 实现通常会把 bucket 数量主动调大到足够多；
   - 这一步本身就可能触发一次 rehash（把已有元素搬到新桶数组）。

2. `rehash(n)`：

   ```cpp
   umap.rehash(200);  // 至少 200 个 bucket
   ```

   - 显式要求至少有 `n` 个桶；
   - 如需增桶，会立刻做一次 rehash。

这两种都属于“你主动要求它换桶”。

---

## 3. 哪些操作**不会**自动 rehash？

面试容易挖坑的地方是“删元素会不会触发 rehash”：

- `erase` / `clear`：
  - **通常不会自动缩小 bucket 数量**，只减少 `size()`；
  - 也就是说，“删很多元素之后，`bucket_count()` 一般不会自动降下来”，负载因子只是变小。
- 其他不改变元素数量的操作（`find` / `at` / 遍历等）当然也不会 rehash。

如果你想在删很多元素后**主动缩小桶数组**，需要自己调：

```cpp
umap.rehash(new_bucket_count);  // 或构造一个新的再 swap
```

---

## 4. 面试时可以怎么回答“什么时候需要 rehash？” 

可以组织成一段话：

> 对 `std::unordered_map` 来说，rehash 的触发条件主要有两类：  
> 一类是**自动触发**：当插入新元素后，负载因子 `load_factor()` 超过 `max_load_factor()`，容器会自动增加 bucket 数量并 rehash，把所有元素重新分布到新的桶里；  
> 另一类是**手动触发**：调用 `reserve(n)` 或 `rehash(n)` 时，如果需要更多 bucket，也会立刻进行一次 rehash。  
> 删除元素一般不会自动收缩 bucket 数量，所以不会自动 rehash，除非我们显式调用 `rehash` 或重建容器。

如果考官继续追问“为什么要控制 rehash”：

- 你可以补充：
  - 频繁 rehash 有一次 O(n) 的搬家成本，过于频繁会拖慢插入；
  - 提前 `reserve` 可以把多次小 rehash 换成一次大 rehash；
  - rehash 时所有迭代器失效，这也是需要注意的点。