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
    size_t size = 256 * 1024 - 1;
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

    ConcurrentFree(p1);
    ConcurrentFree(p2);
    ConcurrentFree(p3);
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
        ConcurrentFree(p);
}

void TestMultiThread()
{
    std::thread t1(MultiThreadAlloc);
    std::thread t2(MultiThreadAlloc);
    t1.join();
    t2.join();
}

// 验证mmap()返回的指针是不是2^13的倍数
// 不是的话能否通过重新申请获得是2^13的倍数的地址
void TestShift()
{
    std::vector<void*> v;
	for (int i = 0; i < 10; i++)
	{
        void* ptr = mmap(nullptr, (NPAGES - 1) << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        int cnt = 1;
		while (((PAGE_ID)ptr >> 12) % 2 == 1)
		{
			cout << ptr << ": 不是2^13的倍数" << endl;
            munmap(ptr, (NPAGES - 1) << PAGE_SHIFT);
            void* hint  = (void*)((char*)ptr + (NPAGES << PAGE_SHIFT));
            // srand(time(nullptr));
            ptr = mmap(hint, (NPAGES - 1) << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
            cnt++;
            if (cnt > 10)
            {
                cout << "申请失败" << endl;
                return;
            }
		}
        cout << ptr << ":" << cnt << "次申请成功" << endl;
        v.push_back(ptr);
	}
	for (auto ptr : v)
        munmap(ptr, (NPAGES - 1) << PAGE_SHIFT); 
    // void* ptr = (void*)(0x7ffff7157000);
    // cout << "ptr:" << ptr << endl << "ptr >> 13 << 13 = " << endl 
    //      << (void*)(((PAGE_ID)ptr) >> 13 << 13) << endl;
}

void TestBigAlloc()
{
    void* p1 = ConcurrentAlloc(257 * 1024); // 257KB, 33页
    ConcurrentFree(p1);
    void* p2 = ConcurrentAlloc(129 * 8 * 1024); // 129页
    ConcurrentFree(p2);
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
    TestBigAlloc();
    return 0;
}