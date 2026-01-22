#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>

std::queue<int> q;
std::mutex mtx;
std::condition_variable cv;
bool finished = false;


void producer() {
    for (int i = 1; i <= 2; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(i);
        std::cout << "[Producer] Produced " << i << std::endl;
    }
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    for (int i = 3; i <= 6; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(i);
        std::cout << "[Producer] Produced " << i << std::endl;
    }
    cv.notify_one();
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    for (int i = 7; i <= 10; ++i) {
        std::lock_guard<std::mutex> lock(mtx);
        q.push(i);
        std::cout << "[Producer] Produced " << i << std::endl;
    }
    cv.notify_one();

    {
        std::lock_guard<std::mutex> lock(mtx);
        finished = true;
    }

    cv.notify_one();
}

void consumer() {
    while (true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{return !q.empty() || finished; });

        while (!q.empty()) {
            int val = q.front();
            q.pop();
            lock.unlock();
            std::cout << "[Consumer] Consumed " << val << std::endl;
            lock.lock();
        }
        if (finished && q.empty()) break;
    }
}

int main() {
    std::thread prod(producer);
    std::thread cons(consumer);
    prod.join();
    cons.join();
    std::cout << "ALL DONE!" << std::endl;
    return 0; 
}

