#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <chrono>
#include <sstream>

class ThreadPool {
public:
    explicit ThreadPool(size_t thread_count, size_t max_queue_size) 
        : stop(false), max_queue_size(max_queue_size) {
            for (size_t i = 0; i < thread_count; ++i) {
                workers.emplace_back([this, i]{
                    worker_loop(i);
                });
            }
        }
    
    ~ThreadPool() {
        shutdown();
    }

    void submit(std::function<void()> task) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_full.wait(lock,[this] {
            return stop || tasks.size() < max_queue_size;
        });

        if(stop) {
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
    void worker_loop(size_t id){
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
    std::vector<std::thread> workers;
    std::queue<std::function<void()>> tasks;

    std::mutex mtx;
    std::condition_variable cv_not_empty;
    std::condition_variable cv_not_full;

    bool stop;
    size_t max_queue_size;
};

int main() {
    ThreadPool pool(3,5);
    for (int i = 0; i < 10; ++i) {
        pool.submit([i] {
            std::ostringstream oss;
            oss << "Task " << i
                << " executed by thread "
                << std::this_thread::get_id() << '\n';
            std::cout << oss.str();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        });
    }

    pool.shutdown();
}