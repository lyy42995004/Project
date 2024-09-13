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

                _spanList[nSpan->_n].PushFront(nSpan);
                return kSpan;
            }
        }
        // 没有大于k页的span了，向内存申请大页
        Span* bigSpan = new Span;
        bigSpan->_pageID = ((PAGE_ID)SystemAlloc(NPAGES - 1)) >> PAGE_SHIFT;
        bigSpan->_n = NPAGES - 1;
        _spanList[bigSpan->_n].PushFront(bigSpan);

        // 申请128页span后，复用自己
        return NewSpan(k);
    }
private:
    SpanList _spanList[NPAGES];
public:
    std::mutex _mtx;
private:
    PageCache()
    {}
    PageCache(const PageCache&) = delete;
    static PageCache _sInst;
};

PageCache PageCache::_sInst;