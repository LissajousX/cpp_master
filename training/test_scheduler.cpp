#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <future>
#include <chrono>
#include <sstream>
#include <type_traits>
#include <atomic>

// 简化版线程池：有界队列 + future
class ThreadPool {
public:
    ThreadPool(size_t threadCnt, size_t maxQueue)
        : stop(false), max_queue_size(maxQueue) {
        for (size_t i = 0; i < threadCnt; ++i) {
            workers.emplace_back([this]{ worker_loop(); });
        }
    }

    ~ThreadPool() { shutdown(); }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return;
            stop = true;
        }
        cv_not_empty.notify_all();
        cv_not_full.notify_all();
        for (auto &t : workers) {
            if (t.joinable()) t.join();
        }
    }

    template<typename F, typename... Args>
    auto submit(F&& f, Args&&... args)
        -> std::future<typename std::invoke_result<F, Args...>::type>
    {
        using return_type = typename std::invoke_result<F, Args...>::type;

        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...)
        );

        std::future<return_type> res = task->get_future();

        {
            std::unique_lock<std::mutex> lock(mtx);
            cv_not_full.wait(lock, [this]{
                return stop || tasks.size() < max_queue_size;
            });
            if (stop) {
                throw std::runtime_error("ThreadPool stopped");
            }
            tasks.push([task]{ (*task)(); });
        }

        cv_not_empty.notify_one();
        return res;
    }

private:
    void worker_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv_not_empty.wait(lock, [this]{
                    return stop || !tasks.empty();
                });
                if (stop && tasks.empty()) return;
                task = std::move(tasks.front());
                tasks.pop();
                cv_not_full.notify_one();
            }
            task();
        }
    }

private:
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    std::mutex mtx;
    std::condition_variable cv_not_empty;
    std::condition_variable cv_not_full;
    bool stop;
    size_t max_queue_size;
};

// 调度器：支持立即任务、延迟一次任务、周期任务
class Scheduler {
public:
    using Clock = std::chrono::steady_clock;

    explicit Scheduler(ThreadPool &pool)
        : pool(pool), stop(false), scheduler_thread(&Scheduler::run, this) {}

    ~Scheduler() { shutdown(); }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return;
            stop = true;
        }
        cv.notify_all();
        if (scheduler_thread.joinable()) scheduler_thread.join();
    }

    // 立即执行（丢给线程池）
    template<typename F, typename... Args>
    void post(F&& f, Args&&... args) {
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        add_task(Clock::now(), std::move(bound), std::chrono::milliseconds(0));
    }

    // 延迟一次
    template<typename Rep, typename Period, typename F, typename... Args>
    void post_after(const std::chrono::duration<Rep, Period> &delay,
                    F&& f, Args&&... args) {
        auto when = Clock::now() + delay;
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        add_task(when, std::move(bound), std::chrono::milliseconds(0));
    }

    // 周期任务
    template<typename Rep, typename Period, typename F, typename... Args>
    void post_every(const std::chrono::duration<Rep, Period> &interval,
                    F&& f, Args&&... args) {
        auto when = Clock::now() + interval;
        auto bound = std::bind(std::forward<F>(f), std::forward<Args>(args)...);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(interval);
        add_task(when, std::move(bound), ms);
    }

private:
    struct Item {
        Clock::time_point when;
        std::function<void()> func;
        std::chrono::milliseconds interval; // 0 表示一次性
        std::size_t seq;                    // 打破同一时间的并列
    };

    struct Cmp {
        bool operator()(const Item &a, const Item &b) const {
            if (a.when != b.when) return a.when > b.when; // 小顶堆
            return a.seq > b.seq;
        }
    };

    void add_task(Clock::time_point when,
                  std::function<void()> func,
                  std::chrono::milliseconds interval) {
        std::lock_guard<std::mutex> lock(mtx);
        items.push(Item{when, std::move(func), interval, seq++});
        cv.notify_one();
    }

    void run() {
        std::unique_lock<std::mutex> lock(mtx);
        while (!stop) {
            if (items.empty()) {
                cv.wait(lock, [this]{ return stop || !items.empty(); });
                if (stop) break;
            } else {
                auto now = Clock::now();
                auto next = items.top().when;
                if (next > now) {
                    cv.wait_until(lock, next, [this]{ return stop; });
                    if (stop) break;
                    continue; // 被时间唤醒后再检查
                }
                // 取出到期任务
                Item item = items.top();
                items.pop();
                lock.unlock();

                // 丢给线程池执行
                pool.submit(item.func);

                // 如果是周期任务，重新安排下一次
                if (item.interval.count() > 0 && !stop) {
                    item.when = Clock::now() + item.interval;
                    std::lock_guard<std::mutex> relock(mtx);
                    items.push(item);
                    cv.notify_one();
                }

                lock.lock();
            }
        }
    }

private:
    ThreadPool &pool;
    std::priority_queue<Item, std::vector<Item>, Cmp> items;
    std::mutex mtx;
    std::condition_variable cv;
    bool stop;
    std::thread scheduler_thread;
    std::size_t seq{0};
};

int main() {
    ThreadPool pool(3, 16);
    Scheduler sched(pool);

    // 立即任务
    for (int i = 0; i < 3; ++i) {
        sched.post([i]{
            std::cout << "immediate task " << i
                      << " on thread " << std::this_thread::get_id() << "\n";
        });
    }

    // 延迟一次任务
    sched.post_after(std::chrono::seconds(1), []{
        std::cout << "delayed 1s task on thread "
                  << std::this_thread::get_id() << "\n";
    });

    // 周期任务：每 500ms 打印一次，总共大约跑几次
    std::atomic<int> counter{0};
    sched.post_every(std::chrono::milliseconds(500), [&counter]{
        int c = ++counter;
        std::cout << "periodic task tick " << c
                  << " on thread " << std::this_thread::get_id() << "\n";
    });

    // 主线程睡 3 秒，观察输出
    std::this_thread::sleep_for(std::chrono::seconds(3));

    sched.shutdown();
    pool.shutdown();
}
