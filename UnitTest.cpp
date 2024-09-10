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

int main()
{
    // TestObjectPool();
    TestTLS();
    return 0;
}