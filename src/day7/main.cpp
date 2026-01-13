#include <iostream>
#include <string>

#include "lru_cache.hpp"

using day7::LRUCache;

int main() {
    LRUCache<int, std::string> cache(3);

    cache.put(1, "one");
    cache.put(2, "two");
    cache.put(3, "three");

    auto v1 = cache.get(1); // 访问 1，使之变成最近使用
    std::cout << "get(1): " << (v1 ? *v1 : "<miss>") << "\n";

    cache.put(4, "four"); // 现在 2 是最久未使用，应被淘汰

    auto v2 = cache.get(2);
    auto v3 = cache.get(3);
    auto v4 = cache.get(4);

    std::cout << "get(2): " << (v2 ? *v2 : "<miss>") << "\n";
    std::cout << "get(3): " << (v3 ? *v3 : "<miss>") << "\n";
    std::cout << "get(4): " << (v4 ? *v4 : "<miss>") << "\n";

    // 演示 put_if_absent
    bool inserted = cache.put_if_absent(3, "THREE");
    std::cout << "put_if_absent(3) inserted? " << std::boolalpha << inserted << "\n";

    auto v3b = cache.get(3);
    std::cout << "get(3) after put_if_absent: " << (v3b ? *v3b : "<miss>") << "\n";

    return 0;
}
