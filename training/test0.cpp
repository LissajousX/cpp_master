#include <cstddef>
#include <algorithm>
#include <vector>
//默认拷贝构造和拷贝赋值是浅拷贝，可能引起多重释放等问题。
struct BadBuffer {
    int* data;
    size_t size;

    BadBuffer(size_t n)
        : data(new int[n]), size(n) {}

    ~BadBuffer() {
        delete[] data;
    }
};



//按照ruleof3来对错误实现进行改造，基本正确，但是没有考虑异常安全
struct Buffer_r3 {
    int* data;
    size_t size;

    Buffer_r3(size_t n)
        : data(new int[n]), size(n) {}

    Buffer_r3(const Buffer_r3& other) 
        : data(new int[other.size]), size(other.size) {
            std::copy(other.data, other.data+size, data);
    }

    Buffer_r3& operator=(const Buffer_r3& other) {
        if (this != &other) {
            delete[] data;
            data = new int[other.size]; //可能抛异常导致半死状态
            size = other.size;
            std::copy(other.data, other.data+size, data);
        }
        return *this;
    }

    ~Buffer_r3() {
        delete[] data;
    }
};

//按照ruleof3来对错误实现进行改造，无异常安全问题

struct Buffer_r3_good {
    int* data;
    size_t size;

    Buffer_r3_good(size_t n)
        : data(new int[n]), size(n) {}

    Buffer_r3_good(const Buffer_r3_good& other) 
        : data(new int[other.size]), size(other.size) {
            std::copy(other.data, other.data+size, data);
    }

    Buffer_r3_good& operator=(const Buffer_r3_good& other) {
        if (this != &other) {
            int* tmp = new int[other.size]; 
            std::copy(other.data, other.data+size, tmp);
            delete[] data;
            data = tmp;
            size = other.size;
        }
        return *this;
    }

    ~Buffer_r3_good() {
        delete[] data;
    }
};

// copy and swap

struct  Buffer_cps
{
    int* data;
    size_t size;
    Buffer_cps(size_t n)
        :data(n > 0 ? new int[n] : nullptr), size(n) {};
    ~Buffer_cps() {
        delete[] data;
    }

    Buffer_cps(const Buffer_cps& other) 
        :data(other.size>0 ? new int[other.size] : nullptr), size(other.size) {
            std::copy(other.data, other.data+size, data);
        }
    
    Buffer_cps& operator=(Buffer_cps other) {
        swap(other);
    }

    void swap(Buffer_cps& other) noexcept {
        std::swap(data, other.data);
        std::swap(size, other.size);
    }
};


struct Buffer_r5_good {
    int* data;
    size_t size;

    Buffer_r5_good(size_t n)
        : data(new int[n]), size(n) {}

    Buffer_r5_good(const Buffer_r5_good& other) 
        : data(new int[other.size]), size(other.size) {
            std::copy(other.data, other.data+size, data);
    }

    Buffer_r5_good& operator=(const Buffer_r5_good& other) {
        if (this != &other) {
            int* tmp = new int[other.size]; 
            std::copy(other.data, other.data+size, tmp);
            delete[] data;
            data = tmp;
            size = other.size;
        }
        return *this;
    }

    Buffer_r5_good(Buffer_r5_good&& other) noexcept 
        : data(other.data), size(other.size) {
            other.data = nullptr;
            other.size = 0;
    }

    Buffer_r5_good& operator=(Buffer_r5_good&& other) noexcept{
        if (this != &other) {
            delete[] data;
            data = other.data;
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }

    ~Buffer_r5_good() {
        delete[] data;
    }
};


struct  Buffer_cps_r5
{
    int* data;
    size_t size;
    Buffer_cps_r5(size_t n)
        :data(n ? new int[n] : nullptr), size(n) {};
    ~Buffer_cps_r5() {
        delete[] data;
    }

    Buffer_cps_r5(const Buffer_cps_r5& other) 
        :data(other.size ? new int[other.size] : nullptr), size(other.size) {
            std::copy(other.data, other.data+size, data);
        }

    
    Buffer_cps_r5(Buffer_cps_r5&& other) noexcept
        :data(other.data), size(other.size) {
            other.data = nullptr;
            other.size = 0;
        }
    
    
    Buffer_cps_r5& operator=(Buffer_cps_r5 other) noexcept{
        swap(other);
    }

    void swap(Buffer_cps_r5& other) noexcept {
        std::swap(data, other.data);
        std::swap(size, other.size);
    }
};


struct Buffer {
    std::vector<int> data;
    Buffer(size_t n) : data(n) {}
};





