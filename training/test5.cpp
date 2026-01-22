#include <iostream>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <vector>

std::queue<int> q;
std::mutex mtx;
std::condition_variable cv;

int producers_left = 0;

void producer(int id, int start, int count) {
    for (int i = 0; i < count; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            int value = start + i;
            q.push(value);
            std::cout << "[Producer " << id << "] produce " << value << "\n";
        }
        cv.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    {
        std::lock_guard<std::mutex> lock(mtx);
        --producers_left;
    }
    cv.notify_all();
}

void consumer(int id) {
    while(true) {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, []{return !q.empty() || producers_left == 0; });
        while (!q.empty()) {
            int value = q.front();
            q.pop();
            lock.unlock();
            std::cout << "    [Consumer " << id << "] consume " << value << "\n";
            lock.lock();
        }
        if (producers_left == 0) break;
    }
}

int main() {
    const int producer_count = 2;
    const int consumer_count = 2;
    producers_left = producer_count;
    
    std::vector<std::thread> producers;
    std::vector<std::thread> consumers;

    for (int i = 0; i < consumer_count; ++i) {
        consumers.emplace_back(consumer, i);
    }

    for (int i = 0; i < producer_count; ++i) {
        producers.emplace_back(producer, i, i*100, 5);
    }

    for (auto& t : producers) t.join();
    for (auto& t : consumers) t.join();
    
    std::cout << "All done.\n";
}
