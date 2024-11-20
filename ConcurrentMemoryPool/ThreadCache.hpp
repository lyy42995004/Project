#include "Common.hpp"
#include "CentralCache.hpp"

class ThreadCache
{
public:
    // 申请内存对象
    void* Allocate(size_t size)
    {
        assert(size < MAX_BYTES);
        size_t alignSize = SizeClass::RoundUp(size);
        size_t index = SizeClass::Index(size);
        if (!_freeLists[index].isEmpty())
            return _freeLists[index].Pop();
        else
            return FetchFromCentralCache(index, alignSize);
    }
    // 从中心缓存获取对象
    // size是已经对齐过的大小了
	void* FetchFromCentralCache(size_t index, size_t size)
    {
        // 慢开始反馈调节算法
        size_t batchNum = std::min(SizeClass::NumMoveSize(size), _freeLists[index].maxSize());
        if (batchNum == _freeLists[index].maxSize())
            _freeLists[index].maxSize()++;
        // 构建新内存的自由链表
        void* start = nullptr;
        void* end = nullptr;
        size_t actualNum = CentralCache::getInstance()->FetchRangeObj(start, end, batchNum, size);
        assert(actualNum >= 1);
        if (actualNum == 1)
            assert(start == end);
        else // 申请到对象有多个时，将剩余对象挂到自由链表中
            _freeLists[index].PushRange(NextObj(start), end, actualNum - 1);
        return start;
    }
    // 释放内存对象
	void Deallocate(void* ptr, size_t size)
    {
        assert(ptr);
        assert(size <= MAX_BYTES);

        // 头插
        size_t index = SizeClass::Index(size);
        _freeLists[index].Push(ptr);

        // 释放对象过多，自由链表过长，向central cache归还内存
        if (_freeLists[index].Size() >= _freeLists[index].maxSize())
            ListTooLong(_freeLists[index], size);
    }
    // 归回maxSize个的自由链表内存
    void ListTooLong(FreeList& freeList, size_t size)
    {
        void* start = nullptr;
        void* end = nullptr;
        freeList.PopRange(start, end, freeList.maxSize());

        //将取出的对象还给central cache中对应的span
        CentralCache::getInstance()->ReleaseListToSpans(start, size);
    }

private:
    FreeList _freeLists[NFREELISTS]; // 哈希桶
};

static thread_local ThreadCache* pTLSThreadCache = nullptr; // C++11线程局部存储TLS