#pragma once

#include <iostream>
#include <vector>
#include <cassert>
#include <thread>
#include <mutex>
#include <unordered_map>
#include "ObjectPool.hpp"
using std::cout;
using std::endl;

// 小于等于MAX_BYTES，就找thread cache申请
// 大于MAX_BYTES，就直接找page cache或者系统堆申请
static const int MAX_BYTES = 256 * 1024;
// thread cache和central cache自由链表哈希桶的表大小
static const size_t NFREELISTS = 208;
// page cache中哈希桶的个数
static const size_t NPAGES = 129;
// 页大小转换偏移，即一页定义为2^13，也就是8KB
static const size_t PAGE_SHIFT = 13;

#ifdef _WIN32
    #include <windows.h>
#elif __linux__ || __unix__ 
    #include <sys/mman.h>
#endif

static void* SystemAlloc(size_t kPage)
{
#ifdef _WIN32
    void* ptr = VirtualAlloc(0, kPage << PAGE_SHIFT, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#elif __linux__ || __unix__
    void* ptr = mmap(nullptr, kPage << PAGE_SHIFT, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
    if (ptr == nullptr)
		throw std::bad_alloc();
    return ptr;
}

static void SystemFree(void* ptr, size_t kPage)
{
#ifdef _WIN32
    VirtualFree(ptr, 0, MEM_RELEASE);
#elif __linux__ || __unix__
    munmap(ptr, kPage << PAGE_SHIFT); 
#endif
}

// 获取一个内存块中存储的指向下一个内存块的指针 
void*& NextObj(void* obj)
{
    return *(void**)obj;
}

class FreeList
{
public:
    // 头插
    void Push(void* obj)  
    {
        assert(obj);
        NextObj(obj) = _freelist; // 将obj插入链表头部
        _freelist = obj;          // 更新头指针
        _size++;
    }    
    // 从自由链表头部获取一个对象
    void* Pop()
    {
        assert(_freelist);
        void* obj = _freelist;
        _freelist = NextObj(_freelist); // 头删
        _size--;
        return obj;
    }
    // 插入一段范围的对象到自由链表
    void PushRange(void* start, void* end, int n)
    {
        assert(start && end);
        // 头插
        NextObj(end) = _freelist;
        _freelist = start;
        _size += n;
    }
    // 删除一段自由链表
    void PopRange(void*& start, void*& end, size_t n)
    {
        assert (_size >= n);
        start = end = _freelist;
        for (size_t i = 0; i < n - 1; i++)
            end = NextObj(end); 
        _freelist = NextObj(end); // 自由链表指向end的下一个
        NextObj(end) = nullptr;   // end的下一个为空
        _size -= n;
    }
    bool isEmpty()
    {
        return _freelist == nullptr;
    }
    size_t& maxSize()
    {
        return _maxSize;
    }
    size_t Size()
    {
        return _size;
    }
private:
    void* _freelist = nullptr;
    size_t _maxSize = 1;
    size_t _size = 0;
};

//     字节数           对齐数      哈希桶下标
//    [1, 128]             8        [0, 16)
//  (128, 1024]           16       [16, 72)
//  (1024, 1024*8]       128      [72, 128)
// (1024*8, 1024*64]    1024     [128, 184)
//(1024*64, 1024*256] 8*1024     [184, 208)
class SizeClass
{
public:
    // 返回向上对齐后的字节数
    static size_t RoundUp(size_t bytes)
    {
        if (bytes <= 128)
            return _RoundUp(bytes, 8);
        else if (bytes <= 1024)
            return _RoundUp(bytes, 16);
        else if (bytes <= 8 * 1024)
            return _RoundUp(bytes, 128);
        else if (bytes <= 64 * 1024)
            return _RoundUp(bytes, 1024);
        else if (bytes <= 256 * 1024)
            return _RoundUp(bytes, 8 * 1024);
        else
            return _RoundUp(bytes, 1 << PAGE_SHIFT);
    }
    //获取对应哈希桶的下标
    static size_t Index(size_t bytes)
    {
        if (bytes <= 128)
            return _Index(bytes, 3) + 0; // 加上前面的坐标
        else if (bytes <= 1024)
            return _Index(bytes - 128, 4) + 16;
        else if (bytes <= 8 * 1024)
            return _Index(bytes - 1024, 7) + 72;
        else if (bytes <= 64 * 1024)
            return _Index(bytes - 8 * 1024, 10) + 128;
        else if (bytes <= 256 * 1024)
            return _Index(bytes - 64 * 1024, 13) + 184;
        else
        {
            assert(false);
            return -1;
        } 
    }
    // thread cache向central cache获取内存的大小
    static size_t NumMoveSize(size_t size)
    {
        assert(size > 0);

        // 给大内存和小内存适当分配内存
        size_t num = MAX_BYTES / size;
        if (num < 2)
            num = 2;
        else if (num > 512)
            num = 512;
        return num;
    }
    //central cache一次向page cache获取多少页
    static size_t NumMovePage(size_t size)
    {
        size_t num = NumMoveSize(size); // 获取thread cache向central cache的内存数量
        size_t bytes = num * size; // 计算出总共需要的内存

        size_t nPage = bytes >> PAGE_SHIFT;
        if (nPage == 0)
            nPage = 1;
        return nPage;
    }
private:
    // static size_t _RoundUp(size_t bytes, size_t alignNum)
    // {
    //     return (bytes % alignNum ? (bytes / alignNum + 1) * alignNum : bytes);
    // }
    // static size_t _Index(size_t bytes, size_t alignShift)
    // {
    //     size_t alignNum = 1 << alignShift;
    //     return (bytes % alignNum ? bytes / alignNum : bytes / alignNum - 1);
    // }

    //位运算
	static inline size_t _RoundUp(size_t bytes, size_t alignNum)
	{
		return ((bytes + alignNum - 1)&~(alignNum - 1));
	}
    //位运算
    static inline size_t _Index(size_t bytes, size_t alignShift)
	{
		return ((bytes + (1 << alignShift) - 1) >> alignShift) - 1;
	}
};

// 一页大约8k，进程地址空间在不同位下分配的页数不同
#if _WIN64 || __x86_64__
    typedef unsigned long long PAGE_ID;
#elif _WIN32 || __i386__
    typedef size_t PAGE_ID;
#endif

struct Span
{
    PAGE_ID _pageID = 0;       // 大块内存起始页的页号
    size_t _n = 0;             // 页的数量

    Span* _next = nullptr;     // 双向链表
    Span* _prev = nullptr;

    size_t _objSize = 0;       // span的大小
    size_t _useCount = 0;      // 被使用的页数
    void* _freeList = nullptr; // 被切好的小块内存的自由链表

    bool _isUse = false;       // 判断是否被central cache使用
};

// 双向带头链表由于管理每一个哈希桶
class SpanList
{
public:
    SpanList()
    {
        _head = new Span;
        // _head = _spanPool.New();
        _head->_next = _head;
        _head->_prev = _head;
    }
    void Insert(Span* pos, Span* newNode)
    {
        assert(pos);
        assert(newNode);

        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = newNode;
        newNode->_prev = prev;
        newNode->_next = next;
        next->_prev = newNode;
    }
    void PushFront(Span* span)
    {
        Insert(Begin(), span);
    }
    void Erase(Span* pos)
    {
        assert(pos);
        assert(pos != _head);

        Span* prev = pos->_prev;
        Span* next = pos->_next;
        prev->_next = next;
        next->_prev = prev;
    }
    Span* PopFront()
    {
        Span* front = _head->_next;
        Erase(front);
        return front;
    }
    bool isEmpty()
    {
        return _head == _head->_next;
    }
    // 模拟迭代器
    Span* Begin()
    {
        return _head->_next; // 第一个节点
    }
    Span* End()
    {
        return _head; // 最后一个节点的下一个
    }
private:
    Span* _head;
    // ObjectPool<Span> _spanPool;
public:
    std::mutex _mtx; // 每一个哈希桶有一个锁
};