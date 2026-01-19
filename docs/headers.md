# C++ 常用头文件功能概览

下面按**功能类别**整理常用标准头文件，主要基于 C++11 之后（含）标准。你可以把这份文档当成速查表。

---

## 一、输入输出相关

- **`<iostream>`**  
  - 标准流：`std::cin`, `std::cout`, `std::cerr`, `std::clog`  
  - 流对象：`std::istream`, `std::ostream`, `std::iostream`  
  - 支持 `<<`、`>>` 运算符进行格式化输入输出

- **`<iomanip>`**  
  - 格式控制：`std::setw`, `std::setfill`, `std::setprecision`, `std::fixed`, `std::scientific`, `std::hex`, `std::dec`, `std::oct`  
  - 用于精确控制输出格式（对齐、小数位数等）

- **`<fstream>`**  
  - 文件 IO：`std::ifstream`, `std::ofstream`, `std::fstream`  
  - 打开模式：`std::ios::in`, `std::ios::out`, `std::ios::binary`, `std::ios::app` 等  

- **`<sstream>`**  
  - 字符串流：`std::istringstream`, `std::ostringstream`, `std::stringstream`  
  - 常用于字符串与数字之间的格式化转换、解析一行文本等

---

## 二、字符串与字符处理

- **`<string>`**  
  - `std::string` 和 `std::wstring`  
  - 常用成员函数：`size`, `substr`, `find`, `replace`, `append`, `c_str` 等  

- **`<cstring>`**（C 风格字符串）  
  - 函数：`std::strlen`, `std::strcmp`, `std::strcpy`, `std::strncpy`, `std::strcat`, `std::memcpy`, `std::memset` 等  

- **`<cctype>`**（字符分类与转换）  
  - 函数：`std::isalpha`, `std::isdigit`, `std::isspace`, `std::toupper`, `std::tolower` 等  

---

## 三、容器相关

- **`<vector>`**  
  - 动态数组：`std::vector<T>`  
  - 随机访问、尾部插入删除高效，最常用容器之一  

- **`<array>`**  
  - 固定大小数组：`std::array<T, N>`  
  - 大小在编译期确定，可与 C 数组互操作  

- **`<list>`**  
  - 双向链表：`std::list<T>`  
  - 任意位置插入删除 O(1)，但随机访问慢  

- **`<forward_list>`**  
  - 单向链表：`std::forward_list<T>`（更轻量，C++11）  

- **`<deque>`**  
  - 双端队列：`std::deque<T>`  
  - 头尾插入删除高效，支持随机访问  

- **`<queue>`**  
  - 适配器：`std::queue<T>`, `std::priority_queue<T>`  
  - 基于底层容器（默认 `deque` 或 `vector`）  

- **`<stack>`**  
  - 栈适配器：`std::stack<T>`  

- **`<set>`** / **`<map>`**  
  - 有序关联容器（基于平衡树）：  
    - `std::set`, `std::multiset`  
    - `std::map`, `std::multimap`  
  - 自动排序，查找/插入/删除为 O(log N)

- **`<unordered_set>`** / **`<unordered_map>`**（C++11）  
  - 无序关联容器（基于哈希表）：  
    - `std::unordered_set`, `std::unordered_multiset`  
    - `std::unordered_map`, `std::unordered_multimap`  
  - 查找/插入平均 O(1)

---

## 四、算法与迭代器

- **`<algorithm>`**  
  - 通用算法：`std::sort`, `std::stable_sort`, `std::find`, `std::count`, `std::binary_search`, `std::lower_bound`, `std::max_element`, `std::for_each`, `std::remove`, `std::unique`, `std::copy`, `std::fill`, `std::transform` 等  
  - 基于迭代器，可作用于各种容器  

- **`<numeric>`**  
  - 数值算法：`std::accumulate`, `std::inner_product`, `std::partial_sum`, `std::iota` 等  

- **`<iterator>`**  
  - 迭代器适配器：`std::back_inserter`, `std::front_inserter`, `std::istream_iterator`, `std::ostream_iterator`  
  - 迭代器 traits：`std::iterator_traits`  

---

## 五、实用工具与语言特性

- **`<utility>`**  
  - `std::pair`, `std::make_pair`, `std::move`, `std::forward`, `std::swap`, `std::declval` 等  
  - 移动语义与完美转发的关键头文件  

- **`<functional>`**  
  - `std::function`, `std::bind`, `std::ref`  
  - 函数对象、回调封装，预定义函数对象如 `std::plus`, `std::less` 等  

- **`<tuple>`**  
  - `std::tuple`, `std::make_tuple`, `std::get`, `std::tie`  
  - 多值返回、聚合不同类型  

- **`<memory>`**  
  - 智能指针：`std::unique_ptr`, `std::shared_ptr`, `std::weak_ptr`, `std::make_shared`  
  - `std::allocator`, `std::addressof` 等  

- **`<type_traits>`**  
  - 编译期类型判断与变换：`std::is_same`, `std::is_integral`, `std::enable_if`, `std::remove_reference` 等  
  - 模板元编程基础  

- **`<chrono>`**  
  - 时间库：`std::chrono::seconds`, `milliseconds`, `steady_clock`, `system_clock`, `high_resolution_clock`  
  - 获取时间点、计算时间差，用于计时、超时控制  

- **`<random>`**  
  - 随机数：`std::mt19937`（梅森旋转算法）、`std::uniform_int_distribution`, `std::normal_distribution` 等  

- **`<optional>`**（C++17）  
  - `std::optional<T>`：表示“可能没有值”的结果，替代返回指针或特殊值  

- **`<variant>`**（C++17）  
  - `std::variant<Ts...>`：类型安全的联合体，多种可能类型之一  

- **`<any>`**（C++17）  
  - `std::any`：类型擦除的万能容器，运行期存放任意类型  

---

## 六、异常与错误处理

- **`<exception>`** / **`<stdexcept>`**  
  - 异常基类与常见异常：`std::exception`, `std::runtime_error`, `std::logic_error`, `std::out_of_range`, `std::invalid_argument` 等  

- **`<system_error>`**  
  - `std::error_code`, `std::system_error`：封装系统错误码，配合文件、线程等系统调用使用  

- **`<cassert>`**  
  - `assert(expr)` 宏：调试断言，在 NDEBUG 定义时失效  

---

## 七、多线程与并发（C++11+）

- **`<thread>`**  
  - `std::thread`：创建和管理线程  

- **`<mutex>`**  
  - `std::mutex`, `std::recursive_mutex`, `std::lock_guard`, `std::unique_lock`  

- **`<condition_variable>`**  
  - `std::condition_variable`, `std::condition_variable_any`：线程条件同步  

- **`<future>`**  
  - 异步任务与期物：`std::async`, `std::future`, `std::promise`, `std::packaged_task`  

- **`<atomic>`**  
  - 原子类型：`std::atomic<T>`，无锁线程安全操作  

---

## 八、数学与数值

- **`<cmath>`**  
  - 数学函数：`std::sin`, `std::cos`, `std::sqrt`, `std::pow`, `std::log`, `std::exp` 等  

- **`<cstdlib>`**  
  - 一些 C 标准库函数：`std::rand`, `std::srand`, `std::abs`, `std::malloc`, `std::free`, `std::exit` 等  
  - 注意：在现代 C++ 中更推荐 `new/delete` 或智能指针，随机数用 `<random>`  

- **`<limits>`**  
  - 类型极值：`std::numeric_limits<T>::max()`, `min()`, `epsilon()` 等  

---

## 九、时间、系统与文件系统（C++17+）

- **`<ctime>`**  
  - C 风格时间：`std::time_t`, `std::time`, `std::localtime`, `std::strftime`  

- **`<filesystem>`**（C++17）  
  - `std::filesystem::path`, `exists`, `is_directory`, `directory_iterator`, `create_directories`, `file_size` 等  
  - 用于遍历目录、操作文件路径  

---

## 十、输入输出格式扩展与本地化

- **`<locale>`**  
  - 本地化支持：数字、小数点、货币、日期格式等  

- **`<codecvt>`**（C++17 起已弃用，但仍常见）  
  - 宽窄字符转换，如 UTF-8 与 UTF-16 之间  
  - 新代码可以考虑用平台/第三方库（如 ICU）或 C++20 `<format>` 配合其它库  

---

## 十一、调试与辅助

- **`<cassert>`**  
  - 调试断言：`assert`  

- **`<typeinfo>`**  
  - RTTI：`typeid` 运算符，得到 `std::type_info`  
  - 常用于动态类型检查和日志输出  

---

## 十二、C 头文件对应关系（了解即可）

- C 头文件通常有两种形式：
  - 旧式：`<stdio.h>`, `<stdlib.h>`  
  - C++ 包装：`<cstdio>`, `<cstdlib>`  
- 推荐在 C++ 项目中使用 `cxxx` 形式（如 `<cstdio>`），符号位于命名空间 `std::` 中。

---

## 十三、与“定义”相关的头文件（类型与常量）

- **`<cstddef>` / `<stddef.h>`**  
  - 提供若干**通用类型定义**：  
    - `std::size_t` / `size_t`：用于表示对象大小、容器长度、计数等（`sizeof` 的返回类型）  
    - `std::ptrdiff_t` / `ptrdiff_t`：两个指针相减的结果类型（有符号）  
    - 还可能包含 `nullptr_t`（在某些实现中）等  
  - 在 C++ 中推荐包含 `<cstddef>` 并使用 `std::size_t`、`std::ptrdiff_t`。  

- **`<cstdint>` / `<stdint.h>`**（C++11）  
  - 定义**固定宽度整数类型**：  
    - 有符号：`std::int8_t`, `std::int16_t`, `std::int32_t`, `std::int64_t`  
    - 无符号：`std::uint8_t`, `std::uint16_t`, `std::uint32_t`, `std::uint64_t`  
  - 以及对应的最小/最大值常量：`INT32_MAX`, `UINT64_MAX` 等（宏）  
  - 用于写**跨平台、定长**整数代码（比如网络协议、文件格式）。  

- **`<climits>` / `<limits.h>`**  
  - 提供**整型范围相关的宏定义**：  
    - `CHAR_BIT`：一个 `char` 有多少比特  
    - `INT_MAX`, `INT_MIN`, `LONG_MAX`, `ULONG_MAX` 等  
  - 与模板方式的 `std::numeric_limits<T>` 相比，它是 C 风格宏，针对基本整型类型。  

- **`<cfloat>` / `<float.h>`**  
  - 提供**浮点类型范围与精度**相关宏：  
    - `FLT_MAX`, `FLT_MIN`, `FLT_EPSILON`（`float` 的最大/最小/精度）  
    - `DBL_MAX`, `DBL_MIN`, `DBL_EPSILON`（`double`）等  
  - 用于对浮点数极值、精度有严格需求的场景。  

> 实战建议：
> - 涉及**大小/下标/长度**一律考虑用 `std::size_t`（来自 `<cstddef>`）。
> - 需要**精确位宽**时用 `<cstdint>` 中的 `std::uint32_t` / `std::int64_t` 等。
> - 需要检查**类型极值**时，优先用 `<limits>` 里的 `std::numeric_limits<T>`；如果是 C 风格接口或宏，使用 `<climits>` / `<cfloat>`.