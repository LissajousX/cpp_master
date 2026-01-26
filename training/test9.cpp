#include <iostream>
#include <mutex>
#include <condition_variable>
#include <thread>
#include <functional>
#include <queue>
#include <vector>
#include <sstream>

class ThreadPool {
public:
    ThreadPool(size_t threadCnt, size_t taskCnt)
        : stop(false), tasksSize(taskCnt) {
            for (int i = 0; i < threadCnt; ++i) {
                workers.emplace_back([this]{
                    work_loop();
                });
            }
        }
    
    ~ThreadPool() {
        shut_down();
    }

    void shut_down() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return;
            stop = true;
        }
        cv_not_empty.notify_all();
        cv_not_full.notify_all();

        for (auto& t : workers) {
            if (t.joinable()) t.join();
        }

    }

    void submit(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_full.wait(lock, [this] {
            return stop || tasks.size() < tasksSize;
        });
        if (stop) {
            throw std::runtime_error("ThreadPool stopped!");
        }
        tasks.push(std::move(task));
        cv_not_empty.notify_one();
    }

private:
    void work_loop() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv_not_empty.wait(lock, [this] {
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
    std::mutex mtx;
    std::condition_variable cv_not_full;
    std::condition_variable cv_not_empty;
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;
    bool stop;
    size_t tasksSize;
};

int main() {
    ThreadPool t(3,5);
    for (int i = 0; i < 10; ++i) {
        t.submit([i] {
            std::ostringstream oss;
            oss << "Task " << i
                << " executed by thread "
                << std::this_thread::get_id() << '\n';
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }
    t.shut_down();
}