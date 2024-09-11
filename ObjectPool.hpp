#pragma once
#include "Common.hpp"
static const int NUM = 128 * 1024;

template <class T>
class ObjectPool
{
public:
    T *New()
    {
        T* obj = nullptr;
        if (!_freeList.isEmpty())
        {
			void* tmp = _freeList.Pop();
            obj = (T*)tmp;
        }
        else
        {
            // 一个obj的大小不能小于它的地址
            // int sz = sizeof(T);
            // int objSize = max(sz, sizeof(void*));
            size_t objSize = sizeof(T) < sizeof(void*) ? sizeof(void*) : sizeof(T);
            // 剩余字节不够一个T类型时扩容
            if (_remainBytes < objSize)
            {
                _remainBytes = NUM; // 剩余不够的大小舍弃?
                _memory = (char *)malloc(NUM);
                if (_memory == nullptr)
                    throw std::bad_alloc();
            }
            obj = (T*)_memory;
            _memory += objSize;
            _remainBytes -= objSize;
        }
        // 定位new，构造T类型
        new(obj)T; 
        return obj;
    }
    void Delete(T* obj)
    {
        // 调用析构清理定位new构造的obj
        obj->~T(); 

		_freeList.Push(obj);
    }

private:
    char *_memory = nullptr;   // 指向未被使用的内存
    int _remainBytes = 0;      // 剩余可使用字节数
	FreeList _freeList;        // 用于管理自由链表
};

// 测试

struct TreeNode
{
	int _val;
	TreeNode* _left;
	TreeNode* _right;
	TreeNode()
		:_val(0)
		, _left(nullptr)
		, _right(nullptr)
	{}
};

void TestObjectPool()
{
	// 申请释放的轮次
	const size_t Rounds = 3;
	// 每轮申请释放多少次
	const size_t N = 1000000;
	std::vector<TreeNode*> v1;
	v1.reserve(N);

	//malloc和free
	size_t begin1 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v1.push_back(new TreeNode);
		}
		for (int i = 0; i < N; ++i)
		{
			delete v1[i];
		}
		v1.clear();
	}
	size_t end1 = clock();

	//定长内存池
	ObjectPool<TreeNode> TNPool;
	std::vector<TreeNode*> v2;
	v2.reserve(N);
	size_t begin2 = clock();
	for (size_t j = 0; j < Rounds; ++j)
	{
		for (int i = 0; i < N; ++i)
		{
			v2.push_back(TNPool.New());
		}
		for (int i = 0; i < N; ++i)
		{
			TNPool.Delete(v2[i]);
		}
		v2.clear();
	}
	size_t end2 = clock();

	cout << "new cost time:" << end1 - begin1 << endl;
	cout << "object pool cost time:" << end2 - begin2 << endl;
}