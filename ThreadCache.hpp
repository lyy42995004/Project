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
    // 释放内存对象
	void Deallocate(void* ptr, size_t size)
    {
        assert(ptr);
        assert(size <= MAX_BYTES);

        // 头插
        size_t index = SizeClass::Index(size);
        _freeLists[index].Push(ptr);
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
            _freeLists[index].PushRange(NextObj(start), end);
        return start;
    }

private:
    FreeList _freeLists[NFREELISTS]; // 哈希桶
};

static thread_local ThreadCache* pTLSThreadCache = nullptr; // C++11线程局部存储TLS