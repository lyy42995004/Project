#pragma once

#include "Common.hpp"
#include "PageCache.hpp"

// 单例模式
class CentralCache
{
public:
    static CentralCache* getInstance()
    {
        return &_sInst;
    }
    // 从central cache获取一定数量的对象给thread cache
    // n申请的内存块的数量
	size_t FetchRangeObj(void*& start, void*& end, size_t n, size_t size)
    {
        size_t index = SizeClass::Index(size);
        _SPanList[index]._mtx.lock(); // 加锁
        Span* span = _sInst.GetOneSpan(_SPanList[index], size);
        assert(span); 
        assert(span->_freeList);

        size_t actualNum = 1;
        start = end = span->_freeList;
        while (NextObj(end) && n - 1) // 下一个节点不能为空，取够即可
        {
            end = NextObj(end);
            actualNum++;
            n--;
        }
        span->_useCount += actualNum;   // 记录使用的数量
        span->_freeList = NextObj(end); // 链接_freeList
        NextObj(end) = nullptr;         // end的下一个节点置空
        
        _SPanList[index]._mtx.unlock(); // 解锁
        return actualNum;
    }
    // 获取一个非空的span
	Span* GetOneSpan(SpanList& spanList, size_t size)
    {
        // 1.先找有无非空的Span
        Span* it = spanList.Begin();
        while (it < spanList.End())
        {
            if (it->_freeList != nullptr)
                return it;
            else
                it = it->_next;
        }

        // 2.向Page Cache申请Span
        //先把central cache的桶锁解掉，其他对象释放内存对象回来，不会阻塞
        spanList._mtx.unlock();

        PageCache::GetInstance()->_mtx.lock();
        Span* newSpan = PageCache::GetInstance()->NewSpan(SizeClass::NumMovePage(size)); 
        PageCache::GetInstance()->_mtx.unlock();
        //新申请的span其他线程暂时看不到，无需加锁

        // 大块内存的起始地址和内存大小
        char* start = (char*)(newSpan->_pageID << PAGE_SHIFT); 
        size_t bytes = newSpan->_n;
        char* end = start + bytes;
         
        // 把大块内存切成自由链表链接起来
        // 尾插，可以提高该线程CPU缓存利用率
        newSpan->_freeList = start;
        start += size;
        void* tail = newSpan->_freeList;
        while (start < end)
        {
            NextObj(tail) = start;
            tail = NextObj(tail);
            start += size;
        }
        NextObj(tail) = nullptr;

        spanList._mtx.lock();
        spanList.PushFront(newSpan);

        return newSpan;
    }
private:
    SpanList _SPanList[NFREELISTS];
private:
    static CentralCache _sInst;
    CentralCache()
    {}
    CentralCache(const CentralCache&) = delete;
};

CentralCache CentralCache::_sInst;