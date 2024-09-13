#pragma once
// 解决多线程同时申请属于自己的ThreadCache时的线程安全问题
#include "ThreadCache.hpp"

// 调用alloc
static void* ConcurrentAlloc(size_t size)
{
    assert(size <= MAX_BYTES);
    // 通过TLS 每个线程无锁的获取自己的专属的ThreadCache对象
    if (pTLSThreadCache == nullptr)
        pTLSThreadCache = new ThreadCache;
    // cout << std::this_thread::get_id() << ":" << pTLSThreadCache << endl; // 调试
    return pTLSThreadCache->Allocate(size);
}
static void ConcurrentFree(void* ptr, size_t size)
{
    assert(pTLSThreadCache);

    pTLSThreadCache->Deallocate(ptr, size);
}