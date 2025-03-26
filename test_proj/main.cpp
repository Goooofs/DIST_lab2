#include <iostream>
#include <chrono>
#include <new>
#include <cstdlib>
#include <ctime>
#include "Allocator.h"

constexpr size_t blockCount = 2'000'000'000;
constexpr size_t blockSize = sizeof(double);

void out_of_memory() {
    std::cerr << "ERROR: Allocation failed — Out of memory!" << std::endl;
    std::abort();
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
    auto arr = new (std::nothrow) MyObjectHeapBlocks*[count];
    if (!arr) out_of_memory();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        arr[i] = new MyObjectHeapBlocks(static_cast<double>(rand() % 10000) / 100.0);
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
    auto arr = new (std::nothrow) MyObjectHeapPool*[count];
    if (!arr) out_of_memory();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        arr[i] = new MyObjectHeapPool(static_cast<double>(rand() % 10000) / 100.0);
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

    if (!staticPoolMemory) {
        std::cerr << "STATIC_POOL: staticPoolMemory not allocated!" << std::endl;
        out_of_memory();
    }

    auto arr = new (std::nothrow) MyObjectStaticPool*[count];
    if (!arr) out_of_memory();

    auto start = std::chrono::high_resolution_clock::now();
    for (size_t i = 0; i < count; ++i) {
        arr[i] = new MyObjectStaticPool(static_cast<double>(rand() % 10000) / 100.0);
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
    std::set_new_handler(out_of_memory);
    std::srand(std::time(nullptr));

    std::cout << "=== Allocator — Test ===" << std::endl;
    std::cout << "Block size: " << blockSize << ", Block count: " << blockCount << "\n";

    if (!staticPoolMemory) out_of_memory();

    testHeapBlocksMode(blockCount);
    testHeapPoolMode(blockCount);
    testStaticPoolMode(blockCount);

    delete[] staticPoolMemory;

    std::cout << "\n=== Finished ===" << std::endl;
    return 0;
}
