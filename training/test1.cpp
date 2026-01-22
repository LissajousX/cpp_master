// 1. 写一个 max_of_two 模板函数：max_of_two(const T& a, const T& b) 返回较大值
template <typename T> 
const T& max_of_two(const T& a, const T& b) {
    return a > b ? a : b;
}

// 2. 写一个 clamp(int x, int low, int high)
int clamp(int x, int low, int high) {
     assert(low <= high && "invalid clamp range");
     if (x < low) return low;
     if (x > high) return high;
     return x;
}

// 3. 写一个 join_strings(const std::vector<std::string>& v, const std::string& sep)
std::string join_strings(const std::vector<std::string>& v, const std::string& sep) {
    std::string ans;
    if (v.empty()) return ans;
    size_t n = v.size();
    for (size_t i = 0; i < n; ++i) {
        ans += v[i];
        if (i < n-1) ans += sep;
    }
    return ans;
}

// 4. 写一个 print_vector(const std::vector<int>& v)
//    要求：按 "1, 2, 3" 这种格式打印所有元素，末尾不要多余的逗号和空格。

void print_vector(const std::vector<int>& v) {
    size_t n = v.size(); 
    for (size_t i = 0; i < n; ++i) {
        if (i > 0) cout << ", ";
        cout << v[i];
    }
}

// 5. 写一个 find_first_even(const std::vector<int>& v)，返回 std::optional<int>
//    要求：找到第一个偶数就返回它；如果没有偶数，则返回 std::nullopt。
std::optional<int> find_first_even(const std::vector<int>& v){
    for (auto i : v) {
        if (i % 2 == 0) return i;
    }
    return std::nullopt;
}

// 6. 写一个 remove_odd(std::vector<int>& v)，移除所有奇数
//    要求：使用 std::remove_if + erase 惯用法，把 v 里所有奇数移除，剩下偶数，原地修改 v。
void remove_odd(std::vector<int>& v) {
    v.erase(std::remove_if(v.begin(), v.end(), [](int x){return x%2!=0;}), v.end());
}

// 7. 写一个 count_words(const std::string& s)
//    要求：按空格分隔单词，返回单词个数（简单版，只把一个或多个连续空格当分隔符）。

int count_words(const std::string& s) {
    int f = 0;
    int ans = 0;
    for (auto c : s) {
        if (c != ' ') {
            f = 1;
        } else if (f) {
            ans++;
            f = 0;
        }
    }

    return ans+f;
}

// 8. 写一个 starts_with(std::string_view s, std::string_view prefix)
//    要求：若 s 以 prefix 开头返回 true，否则返回 false，不使用 C++20 内置 starts_with。
bool starts_with(std::string_view s, std::string_view prefix) {
    if (prefix.size() > s.size()) return false;
    for (size_t i = 0; i < prefix.size(); ++i) {
        if (s[i] != prefix[i]) return false;
    }
    return true;
}

// 9. 写一个 parse_bool(std::string_view s)，返回 std::optional<bool>
//    要求：支持 "true"/"false"/"1"/"0" 四种输入，其它字符串返回 std::nullopt。
std::optional<bool> parse_bool(std::string_view s) {
    if (s == "true" || s == "1") return true;
    if (s == "false" || s == "0") return false;
    return std::nullopt;
}

// 10. 写一个 trim(std::string_view s)
//     要求：去掉前后空格（只考虑 ' '），返回裁剪后的 string_view；注意避免返回悬垂引用。

std::string_view trim(std::string_view s) {
    size_t l = 0;
    size_t r = s.size();
    while (l < r && s[l] == ' ') ++l;
    while (l < r && s[r-1] == ' ') --r;
    return s.substr(l, r-l);
}
// 11. 写一个简单的 Counter 类
//     要求：支持 inc()/dec()/value()，初始值为 0，dec() 不允许减到 0 以下（可以 clamp 到 0）。
struct Counter
{
    int count=0;
    void inc() {
        count++;
    }
    void dec() {
        if (count) count--;
    }
    int value() const{
        return count;
    }
};

// 12. 写一个 Timer 小类
//     要求：使用 std::chrono::steady_clock；提供 start()/stop()/elapsed_ms() 接口，
//           一次 start/stop 记录一次耗时，elapsed_ms() 返回上一次的毫秒数。
#include <chrono>

class Timer {
public:
    Timer() : running(false), elapsed(0) {}

    // 开始计时
    void start() {
        running = true;
        start_time = std::chrono::steady_clock::now();
    }

    // 停止计时，并记录这次耗时
    void stop() {
        if (running) {
            auto end_time = std::chrono::steady_clock::now();
            elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time).count();
            running = false;
        }
    }

    // 返回上一次 start/stop 的耗时（毫秒）
    long long elapsed_ms() const {
        return elapsed;
    }

private:
    bool running;
    std::chrono::steady_clock::time_point start_time;
    long long elapsed; // 毫秒
};


// 13. 写一个简易 ScopeGuard
//     要求：构造时传入一个 std::function<void()> 或模板可调用对象，析构时自动调用，
//           用来练习 RAII 资源清理。
template <typename Callable>
struct ScopeGuard {
    Callable&& func;
    ScopeGuard(Callable&& func) : func(std::forward<Callable>(func)) {}
    ~ScopeGuard() { func(); }
};
// 14. 写一个最简单的多线程累加 demo
//     要求：全局 int counter = 0；启动两个 std::thread，每个对 counter 执行 100000 次 ++；
//           先写无锁版，观察结果可能不稳定；再写加 std::mutex 版，结果稳定为 200000。


// 15. 写一个 print_thread_id 的小程序
//     要求：启动 3 个线程，每个线程打印自己的线程 id 和一条消息；
//           使用 std::this_thread::get_id() 和 sleep_for 做一点延迟。

// 16. 写一个最小的“生产者-消费者”示例
//     要求：使用 std::queue<int> + std::mutex + std::condition_variable；
//           一个线程 push 1..10，另一个线程 pop 并打印，直到收到所有数据。

// 17. 用 std::sort 排序一个 std::vector<std::string>
//     要求：排序规则为：长度从短到长，相同长度时按字典序排序；写一个函数 sort_strings(...) 完成。

// 18. 写一个 count_greater_than(const std::vector<int>& v, int x)
//     要求：使用 std::count_if 统计 v 中大于 x 的元素个数。

// 19. 写一个 square_inplace(std::vector<int>& v)
//     要求：使用 std::transform 把每个元素替换为它的平方。