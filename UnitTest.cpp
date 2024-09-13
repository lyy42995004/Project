#include "ObjectPool.hpp"
#include "ConcurrentAlloc.hpp"

void Alloc1()
{
    for (size_t i = 0; i < 10; i++)
        ConcurrentAlloc(6);
}

void Alloc2()
{
    for (size_t i = 0; i < 10; i++)
        ConcurrentAlloc(8);
}

// 用于测试TLS
void TestTLS()
{
    std::thread t1(Alloc1);
    std::thread t2(Alloc2);
    t1.join();
    t2.join();
}

// 测试申请内存的流程
void TestConcurrentAlloc1()
{
    void* p1 = ConcurrentAlloc(1);
    void* p2 = ConcurrentAlloc(3);
    void* p3 = ConcurrentAlloc(5);
    void* p4 = ConcurrentAlloc(8);
}

int main()
{
    // TestObjectPool();
    // TestTLS();
    TestConcurrentAlloc1();
    return 0;
}