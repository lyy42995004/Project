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

void TestConcurrentAlloc2()
{
    size_t size = 8 * 8 * 1024;
    for (int i = 0; i < 1024; i++)
        ConcurrentAlloc(size);
    ConcurrentAlloc(size);
}

// 测试释放内存流程
void TestConcurrentFree()
{
    void* p1 = ConcurrentAlloc(1);
    void* p2 = ConcurrentAlloc(3);
    void* p3 = ConcurrentAlloc(5);

    ConcurrentFree(p1, 8);
    ConcurrentFree(p2, 8);
    ConcurrentFree(p3, 8);
}

// 测试多线程下申请释放流程
void MultiThreadAlloc()
{
    std::vector<void*> v;
    for (size_t i = 0; i < 7; i++)
    {
        void* p = ConcurrentAlloc(7);
        v.push_back(p);
    }
    for (auto p : v)
        ConcurrentFree(p, 8);
}

void TestMultiThread()
{
    std::thread t1(MultiThreadAlloc);
    std::thread t2(MultiThreadAlloc);
    t1.join();
    t2.join();
}

// 验证SystemAllocs()返回的指针是不是2^13的倍数
void TestShift()
{
    std::vector<void*> v;
	for (int i = 0; i < 10000; i++)
	{
		void* ptr = SystemAlloc(1);
		v.push_back(ptr);
		unsigned long long p = (unsigned long long)ptr;
		cout << p << endl;
		if ((p >> 12) % 2 == 1)
		{
			cout << "不是2^13的倍数" << endl;
			break;
		}
	}
	for (auto ptr : v)
		SystemFree(ptr, 1);
    // void* ptr = (void*)(0x7ffff7157000);
    // cout << "ptr:" << ptr << endl << "ptr >> 13 << 13 = " << endl 
    //      << (void*)(((PAGE_ID)ptr) >> 13 << 13) << endl;
}

int main()
{
    // TestObjectPool();
    // TestTLS();
    // TestConcurrentAlloc1();
    // TestConcurrentAlloc2();
    // TestConcurrentFree();
    // MultiThreadAlloc();
    // TestMultiThread();
    // TestShift();
    return 0;
}