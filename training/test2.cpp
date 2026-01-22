// 14. 写一个最简单的多线程累加 demo
//     要求：全局 int counter = 0；启动两个 std::thread，每个对 counter 执行 100000 次 ++；
//           先写无锁版，观察结果可能不稳定；再写加 std::mutex 版，结果稳定为 200000。
#include <iostream>
#include <thread>
#include <mutex>
#include <atomic>

std::atomic<int> counterat{0};
int counter = 0;
std::mutex mtx;

void inc() {
    for (int i = 0; i < 100000; i++) {
        counterat++;
        // std::lock_guard<std::mutex> lock(mtx);
        counter++;
    }
}

int main() {
    std::thread t1(inc);
    std::thread t2(inc);
    t1.join();
    t2.join();
    std::cout << "count:" << counter << std::endl;
    std::cout << "counterat:" << counterat << std::endl;
    return 0;
}