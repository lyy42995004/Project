#pragma once

#include "Common.hpp"

// 单例模式
class PageCache
{
public:
    static PageCache* GetInstance()
    {
        return &_sInst;
    }
    //获取一个k页的span
	Span* NewSpan(size_t k)
    {
        assert(k > 0 && k < NPAGES);
        if (!_spanList[k].isEmpty())
            return _spanList[k].PopFront();
        for (int i = k + 1; i < NPAGES; i++)
        {
            if (!_spanList[i].isEmpty())
            {
                Span* nSpan = _spanList[i].PopFront(); 
                Span* kSpan = new Span;

                kSpan->_n = k;
                kSpan->_pageID = nSpan->_pageID;
                nSpan->_n -= k;
                nSpan->_pageID += k;
                
                // 用于page cache合并span时的前后查找
                _idSpanMap[nSpan->_pageID] = nSpan;
                _idSpanMap[nSpan->_pageID + nSpan->_n - 1] = nSpan;

                // 构建pageid与span指针的映射关系，用于central cache回收span
                for (PAGE_ID i = 0; i < kSpan->_n; i++)
                    _idSpanMap[kSpan->_pageID + i] = kSpan;

                _spanList[nSpan->_n].PushFront(nSpan);
                return kSpan;
            }
        }
        // 没有大于k页的span了，向内存申请大页
        Span* bigSpan = new Span;
        // mmap返回的地址是系统页大小（大多数为 4KB，2^12）的整数倍
        // 但不一定是2^13的整数倍，这就会导致左移13位后再右移13位可能会与原来的数不相等
        // 所以将span大小8KB改为4KB（2^12)
        void* ptr = SystemAlloc(NPAGES - 1);
        bigSpan->_pageID = ((PAGE_ID)ptr) >> PAGE_SHIFT;
        bigSpan->_n = NPAGES - 1;
        _spanList[bigSpan->_n].PushFront(bigSpan);

        // 申请128页span后，复用自己
        return NewSpan(k);
    }
    //获取自由链表指针到span的映射
    Span* MapObjectToSpan(void* obj)
    {
        PAGE_ID id = ((PAGE_ID)obj) >> PAGE_SHIFT;
        auto map = _idSpanMap.find(id);
        if (map != _idSpanMap.end())
            return map->second;
        else
        {
            assert(false);
            return nullptr;
        }
    }
    // 释放空闲的span回到PageCache，并合并相邻的span
	void ReleaseSpanToPageCache(Span* span)
    {
        // 对span前后页进行合并，缓解内存碎片问题
        // 向前合并
        while (1)
        {
            PAGE_ID prevID = span->_pageID - 1;
            // 查找是否存在前一个span
            auto map = _idSpanMap.find(prevID);
            if (map == _idSpanMap.end())
                break;
            Span* prevSpan = map->second;
            // span是否被使用
            if (prevSpan->_isUse)
                break;
            // 大于128页的span，page cache无法管理
            if (prevSpan->_n + span->_n > NPAGES - 1)
                break;

            _spanList[prevSpan->_n].Erase(prevSpan);
            span->_pageID = prevSpan->_pageID;
            span->_n += prevSpan->_n;
            delete prevSpan;
        }
        // 向后合并
        while (1)
        {
            PAGE_ID nextID = span->_pageID + span->_n;
            auto map = _idSpanMap.find(nextID);
            if (map == _idSpanMap.end())
                break;
            Span* nextSpan = map->second;
            if (nextSpan->_isUse)
                break;
            if (nextSpan->_n + span->_n > NPAGES - 1)
                break;

            _spanList[nextSpan->_n].Erase(nextSpan);
            span->_n += nextSpan->_n;
            delete nextSpan;
        }
        _spanList[span->_n].PushFront(span);
        span->_isUse = false;

        _idSpanMap[span->_pageID] = span;
        _idSpanMap[span->_pageID + span->_n - 1] = span;
    }
private:
    SpanList _spanList[NPAGES];
    std::unordered_map<PAGE_ID, Span*> _idSpanMap;
public:
    std::mutex _mtx;
private:
    PageCache()
    {}
    PageCache(const PageCache&) = delete;
    static PageCache _sInst;
};

PageCache PageCache::_sInst;