#pragma once

#include <iostream>
#include <vector>
#include <assert.h>
using std::cout;
using std::endl;

//小于等于MAX_BYTES，就找thread cache申请
//大于MAX_BYTES，就直接找page cache或者系统堆申请
static const int MAX_BYTES = 256 * 1024;
//thread cache和central cache自由链表哈希桶的表大小
static const size_t NFREELISTS = 208;

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
    }    
    //从自由链表头部获取一个对象
    void* Pop()
    {
        assert(_freelist);
        void* obj = _freelist;
        _freelist = NextObj(_freelist); // 头删
        return obj;
    }
    bool isEmpty()
    {
        return _freelist == nullptr;
    }
private:
    void* _freelist = nullptr;
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
        {
            assert(false);
            return -1;
        }
    }
    //获取对应哈希桶的下标
    static size_t Index(size_t bytes)
    {
        if (bytes <= 128)
            // return _Index(bytes, 8, 0);
            return _Index(bytes, 8) + 0; // 加上前面的坐标
        else if (bytes <= 1024)
            // return _Index(bytes, 16, 16);
            return _Index(bytes, 16) + 16;
        else if (bytes <= 8 * 1024)
            // return _Index(bytes, 128, 72);
            return _Index(bytes, 128) + 72;
        else if (bytes <= 64 * 1024)
            // return _Index(bytes, 1024, 128);
            return _Index(bytes, 1024) + 128;
        else if (bytes <= 256 * 1024)
            // return _Index(bytes, 8 * 1024, 184);
            return _Index(bytes, 8 * 1024) + 184;
        else
        {
            assert(false);
            return -1;
        } 
    }

private:
    // size_t _RoundUp(size_t bytes, size_t alignNum)
    // {
    //     return (bytes % alignNum ? (bytes / alignNum + 1) * alignNum : bytes);
    // }
    // size_t _Index(size_t bytes, size_t alignNum, size_t pos)
    // {
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