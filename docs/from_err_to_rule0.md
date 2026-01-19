好，这里我**不讲空话，直接“写代码 + 演进”**。
我们用一个**最典型、最容易踩坑的资源：堆数组**，一步步把它从“必炸”写到 **Rule of 0**。

---

# Stage 0：❌ 错误版（教科书级灾难）

```cpp
struct BadBuffer {
    int* data;
    size_t size;

    BadBuffer(size_t n)
        : data(new int[n]), size(n) {}

    ~BadBuffer() {
        delete[] data;
    }
};
```

### 问题

```cpp
BadBuffer a(10);
BadBuffer b = a;   // 默认拷贝
```

* `data` 被浅拷贝
* 析构两次 → **double free（UB）**

👉 **结论：含 owning 裸指针 + 默认拷贝 = 必炸**

---

# Stage 1：🩹 禁用拷贝（最小止血）

```cpp
struct MoveOnlyBuffer {
    int* data;
    size_t size;

    MoveOnlyBuffer(size_t n)
        : data(new int[n]), size(n) {}

    ~MoveOnlyBuffer() {
        delete[] data;
    }

    MoveOnlyBuffer(const MoveOnlyBuffer&) = delete;
    MoveOnlyBuffer& operator=(const MoveOnlyBuffer&) = delete;
};
```

### 效果

* 禁止拷贝
* 至少不 UB 了

### 代价

* 不能按值传递
* 不能放进 `std::vector`

👉 **只是“活下来”，不优雅**

---

# Stage 2：✅ Rule of 3（正确的深拷贝）

```cpp
struct Buffer {
    int* data;
    size_t size;

    Buffer(size_t n)
        : data(new int[n]), size(n) {}

    ~Buffer() {
        delete[] data;
    }

    Buffer(const Buffer& other)
        : data(new int[other.size]), size(other.size) {
        std::copy(other.data, other.data + size, data);
    }

    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            int* tmp = new int[other.size];
            std::copy(other.data, other.data + other.size, tmp);
            delete[] data;
            data = tmp;
            size = other.size;
        }
        return *this;
    }
};
```

### 现在有了

* 正确深拷贝
* 值语义成立

### 但问题还在

* 拷贝代价高
* 临时对象性能差

---

# Stage 3：🚀 Rule of 5（加上移动语义）

```cpp
struct Buffer {
    int* data;
    size_t size;

    Buffer(size_t n)
        : data(new int[n]), size(n) {}

    ~Buffer() {
        delete[] data;
    }

    // 拷贝构造
    Buffer(const Buffer& other)
        : data(new int[other.size]), size(other.size) {
        std::copy(other.data, other.data + size, data);
    }

    // 拷贝赋值
    Buffer& operator=(const Buffer& other) {
        if (this != &other) {
            int* tmp = new int[other.size];
            std::copy(other.data, other.data + other.size, tmp);
            delete[] data;
            data = tmp;
            size = other.size;
        }
        return *this;
    }

    // 移动构造
    Buffer(Buffer&& other) noexcept
        : data(other.data), size(other.size) {
        other.data = nullptr;
        other.size = 0;
    }

    // 移动赋值
    Buffer& operator=(Buffer&& other) noexcept {
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
};
```

### 现在是

* 安全
* 高效
* 可放 STL
* 标准库级 RAII

👉 **这是你“手写智能指针 / 容器”的能力门槛**

---

# Stage 4：✨ Rule of 0（现代 C++ 最终形态）

### 思想转变（关键）

> **不要自己管理资源，让成员对象替你管理**

---

### 改写

```cpp
#include <vector>

struct Buffer {
    std::vector<int> data;

    Buffer(size_t n) : data(n) {}
};
```

### 神奇的事情发生了

你 **没有写**：

* 析构函数
* 拷贝构造
* 拷贝赋值
* 移动构造
* 移动赋值

但你 **自动获得**：

* 正确析构
* 深拷贝
* 高效移动
* 异常安全
* STL 友好

👉 **这就是 Rule of 0**

---

# 进阶：Rule of 0 + 独占资源（`unique_ptr`）

```cpp
#include <memory>

struct Buffer {
    std::unique_ptr<int[]> data;
    size_t size;

    Buffer(size_t n)
        : data(std::make_unique<int[]>(n)), size(n) {}
};
```

* 默认：**不可拷贝**
* 可移动
* 无 UB
* 无自定义析构

---

# 一张“演进总结表”

| 阶段            | 能用 | 安全 | 高效 | 推荐    |
| ------------- | -- | -- | -- | ----- |
| 错误版           | ❌  | ❌  | ❌  | ❌     |
| 禁拷贝           | ⚠️ | ✔  | ✔  | ❌     |
| Rule of 3     | ✔  | ✔  | ❌  | ⚠️    |
| Rule of 5     | ✔  | ✔  | ✔  | ✔     |
| **Rule of 0** | ✔  | ✔  | ✔  | ⭐⭐⭐⭐⭐ |

---

# 一句话工程总结（很硬）

> **RAII 的终极目标不是“把 Rule of 5 写对”，而是“设计到你根本不用写它们”。**

如果你愿意，下一步我可以继续带你：
👉 **从 Rule of 0 反推 `std::vector / shared_ptr` 的设计哲学**
👉 **你自己手写一个 `unique_ptr`（我们已经到这一步了）**
