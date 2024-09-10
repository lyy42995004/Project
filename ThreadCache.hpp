#include "Common.hpp"

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
    //从中心缓存获取对象
	void* FetchFromCentralCache(size_t index, size_t size)
    {
        return nullptr;
    }

private:
    FreeList _freeLists[NFREELISTS]; // 哈希桶
};

static thread_local ThreadCache* pTLSThreadCache = nullptr; // C++11线程局部存储TLS