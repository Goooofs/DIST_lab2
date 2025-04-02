#include <iostream>
#include <chrono>
#include <new>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstddef>
#include <fstream>
#include "Allocator.h"

#if defined(_WIN32)
#include <windows.h>
#elif defined(__linux__)
#include <unistd.h>
#endif

constexpr size_t blockCount = 1'000'000'000;
constexpr size_t blockSize = sizeof(double);

//MEMORY UTILS 
size_t getAvailableRAM() {
#if defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(memInfo);
    if (GlobalMemoryStatusEx(&memInfo)) {
        return static_cast<size_t>(memInfo.ullAvailPhys);
    }
    return 0;
#elif defined(__linux__)
    std::ifstream meminfo("/proc/meminfo");
    std::string line;
    while (std::getline(meminfo, line)) {
        if (line.rfind("MemAvailable:", 0) == 0) {
            size_t kb = std::stoull(line.substr(14));
            return kb * 1024;
        }
    }
    return 0;
#else
    return SIZE_MAX;
#endif
}

size_t getProcessMemoryKB(const std::string& key) {
#if defined(__linux__)
    std::ifstream status("/proc/self/status");
    std::string line;
    while (std::getline(status, line)) {
        if (line.rfind(key, 0) == 0) {
            size_t kb = std::stoull(line.substr(key.size()));
            return kb;
        }
    }
#endif
    return 0;
}

size_t getVmSize() {
    return getProcessMemoryKB("VmSize:\t") * 1024;
}

bool willLikelyOOM(size_t upcomingBytes) {
    size_t available = getAvailableRAM();
    size_t current = getVmSize();
    size_t total = current + upcomingBytes;
    size_t threshold = available * 8 / 10;

    // std::cout << "Available RAM: " << (available >> 20) << " MB" << std::endl;
    // std::cout << "Current VmSize: " << (current >> 20) << " MB" << std::endl;
    // std::cout << "Upcoming alloc: " << (upcomingBytes >> 20) << " MB" << std::endl;
    // std::cout << "Safe threshold: " << (threshold >> 20) << " MB" << std::endl;

    if (total > threshold) {
        std::cerr << "OOM Risk — too much memory requested, skipping.\n";
        return true;
    }
    return false;
}

class MyObject {
public:
    MyObject(double v) : value(v) {}
    ~MyObject() = default;
    double getValue() const { return value; }

private:
    double value;
};

class MyObjectHeapBlocks : public MyObject {
    DECLARE_ALLOCATOR
public:
    MyObjectHeapBlocks(double v) : MyObject(v) {}
};
IMPLEMENT_ALLOCATOR(MyObjectHeapBlocks, 0, 0)

class MyObjectHeapPool : public MyObject {
    DECLARE_ALLOCATOR
public:
    MyObjectHeapPool(double v) : MyObject(v) {}
};
IMPLEMENT_ALLOCATOR(MyObjectHeapPool, blockCount, 0)

static char* staticPoolMemory = new (std::nothrow) char[blockSize * blockCount];
class MyObjectStaticPool : public MyObject {
    DECLARE_ALLOCATOR
public:
    MyObjectStaticPool(double v) : MyObject(v) {}
};
IMPLEMENT_ALLOCATOR(MyObjectStaticPool, blockCount, staticPoolMemory)

void printValues(MyObject* const* arr, size_t count) {
    std::cout << "[";
    for (size_t i = 0; i < count; ++i) {
        std::cout << arr[i]->getValue();
        if (i < count - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

void testHeapBlocksMode(size_t count) {
    std::cout << "\n=== HEAP_BLOCKS Mode ===" << std::endl;

    size_t required = sizeof(MyObjectHeapBlocks*) * count;
    if (willLikelyOOM(required)) return;

    MyObjectHeapBlocks** arr = nullptr;
    try {
        arr = new MyObjectHeapBlocks*[count];
    } catch (const std::bad_alloc&) {
        std::cerr << "HEAP_BLOCKS: Allocation failed.\n";
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        try {
            arr[i] = new MyObjectHeapBlocks(static_cast<double>(rand() % 10000) / 100.0);
        } catch (const std::bad_alloc&) {
            std::cerr << "HEAP_BLOCKS: Allocation failed at index " << i << std::endl;
            count = i;  
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Allocation time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    // printValues(reinterpret_cast<MyObject* const*>(arr), count);

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) delete arr[i];
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Deallocate time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    delete[] arr;
}

void testHeapPoolMode(size_t count) {
    std::cout << "\n=== HEAP_POOL Mode ===" << std::endl;

    size_t required = sizeof(MyObjectHeapPool*) * count;
    if (willLikelyOOM(required)) return;

    MyObjectHeapPool** arr = nullptr;
    try {
        arr = new MyObjectHeapPool*[count];
    } catch (const std::bad_alloc&) {
        std::cerr << "HEAP_POOL: Failed to allocate pointer array\n";
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        try {
            arr[i] = new MyObjectHeapPool(static_cast<double>(rand() % 10000) / 100.0);
        } catch (const std::bad_alloc&) {
            std::cerr << "HEAP_POOL: Allocation failed at index " << i << std::endl;
            count = i;
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Allocation time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    // printValues(reinterpret_cast<MyObject* const*>(arr), count);

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) delete arr[i];
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Deallocate time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    delete[] arr;
}

void testStaticPoolMode(size_t count) {
    std::cout << "\n=== STATIC_POOL Mode ===" << std::endl;

    size_t required = sizeof(MyObjectStaticPool*) * count;
    if (willLikelyOOM(required)) return;

    if (!staticPoolMemory) {
        std::cerr << "STATIC_POOL: Memory not allocated.\n";
        return;
    }

    MyObjectStaticPool** arr = nullptr;
    try {
        arr = new MyObjectStaticPool*[count];
    } catch (const std::bad_alloc&) {
        std::cerr << "STATIC_POOL: Failed to allocate pointer array\n";
        return;
    }

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        try {
            arr[i] = new MyObjectStaticPool(static_cast<double>(rand() % 10000) / 100.0);
        } catch (const std::bad_alloc&) {
            std::cerr << "STATIC_POOL: Allocation failed at index " << i << std::endl;
            count = i;
            break;
        }
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Allocation time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    // printValues(reinterpret_cast<MyObject* const*>(arr), count);

    start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) delete arr[i];
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Deallocate time: " << std::chrono::duration<double, std::milli>(end - start).count() << " ms\n";

    delete[] arr;
}

int main() {
    std::srand(std::time(nullptr));

    std::cout << "=== Allocator — Test ===" << std::endl;
    std::cout << "Block size: " << blockSize << ", Block count: " << blockCount << "\n";

    testHeapBlocksMode(blockCount);
    testHeapPoolMode(blockCount);
    testStaticPoolMode(blockCount);

    delete[] staticPoolMemory;

    std::cout << "\n=== Finished ===" << std::endl;
    return 0;
}
