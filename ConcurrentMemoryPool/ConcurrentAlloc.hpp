#pragma once
// 解决多线程同时申请属于自己的ThreadCache时的线程安全问题
#include "Common.hpp"
#include "ThreadCache.hpp"

// 调用alloc
static void* ConcurrentAlloc(size_t size)
{
    if (size > MAX_BYTES)
    {
        // 计算需要申请页数
        size_t alignSize = SizeClass::RoundUp(size);
        size_t kPage = alignSize >> PAGE_SHIFT;
        // 向page cache申请span
        PageCache::GetInstance()->_mtx.lock();
        Span* span = PageCache::GetInstance()->NewSpan(kPage);
        span->_objSize = size;
        PageCache::GetInstance()->_mtx.unlock();

        void* ptr = (void*)(span->_pageID << PAGE_SHIFT);
        return ptr;
    }
    else
    {
        // 通过TLS 每个线程无锁的获取自己的专属的ThreadCache对象
        if (pTLSThreadCache == nullptr)
        {
            ObjectPool<ThreadCache> tcPool;
            // pTLSThreadCache = new ThreadCache;
            static std::mutex mtx;
            mtx.lock();
            pTLSThreadCache = tcPool.New();
            mtx.unlock();
        }
        // cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl; // 调试
        return pTLSThreadCache->Allocate(size);
    }
}

static void ConcurrentFree(void* ptr)
{
    Span* span = PageCache::GetInstance()->MapObjectToSpan(ptr);
    size_t size = span->_objSize;
    if (size > MAX_BYTES)
    {
        PageCache::GetInstance()->_mtx.lock();
        PageCache::GetInstance()->ReleaseSpanToPageCache(span);
        PageCache::GetInstance()->_mtx.unlock();
    }
    else
    {
        assert(pTLSThreadCache);
        pTLSThreadCache->Deallocate(ptr, size);
    }
}