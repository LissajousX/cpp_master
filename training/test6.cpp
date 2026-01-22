#include <iostream>
#include <queue>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <vector>

constexpr size_t MAX_QUEUE_SIZE = 5;

std::queue<int> q;
std::mutex mtx;
std::condition_variable cv_not_empty;
std::condition_variable cv_not_full;

int producer_left = 0;

void producer(int id, int start, int count) {
    for (int i = 0; i <= count; ++i ) {
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_full.wait(lock, [] { return q.size() < MAX_QUEUE_SIZE;});

        int value = start+i;
        q.push(value);
        std::cout << "[Producer " << id << "] produce " << value << "\n";
        lock.unlock();
        cv_not_empty.notify_one();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        --producer_left;
    }
    cv_not_empty.notify_all();
}

void consumer(int id) {
    while(true){
        std::unique_lock<std::mutex> lock(mtx);
        cv_not_empty.wait(lock, []{return !q.empty() || producer_left == 0;});
        while (!q.empty()) {
            int val = q.front();
            q.pop();
            lock.unlock();
            std::cout << "    [Consumer " << id << "] consume " << val << "\n";
            cv_not_full.notify_one();
            std::this_thread::sleep_for(std::chrono::milliseconds(80));
            lock.lock();
        }

        if (producer_left == 0) break;
    }    
}

int main() {
    int producer_count = 2;
    int consumer_count = 2;
    producer_left = producer_count;

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

    return 0;
}