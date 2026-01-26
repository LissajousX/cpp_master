#include <iostream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <queue>
#include <functional>
#include <sstream>


class ThreadPool {
public:
    ThreadPool(size_t workernum, size_t tasknum)
        : stop(false), max_task_count(tasknum) {
        for (int i = 0; i < workernum; ++i) {
            workers.emplace_back([this]{workloop();});
        }
    }

    ~ThreadPool() {
        shutdown();
    }

    void submit(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_full.wait(lock, [this]{
            return stop || tasks.size() < max_task_count;
        });

        if (stop) {
            throw std::runtime_error("ThreadPool stopped");
        }

        tasks.push(std::move(task));
        cv_not_empty.notify_one();
    }

    void shutdown() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            if (stop) return;
            stop = true;
        }

        cv_not_empty.notify_all();
        cv_not_full.notify_all();

        for (auto& t : workers) {
            if (t.joinable()) {
                t.join();
            }
        }
    }


private:
    void workloop() {
        while(true) {
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
    std::condition_variable cv_not_full;
    std::condition_variable cv_not_empty;

    bool stop;
    size_t max_task_count;
};

int main() {
    ThreadPool t(3,5);
    for (int i = 0; i < 10; i++) {
        t.submit([i] {
            std::ostringstream oss;
            oss << "Task " << i
                << " executed by thread "
                << std::this_thread::get_id() << '\n';
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }
}
