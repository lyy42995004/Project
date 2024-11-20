#pragma once

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Task.hpp"
#include "Log.hpp"

#define NUM 6

class ThreadPool
{
private:
    ThreadPool()
        :_num(NUM)
    {
        pthread_mutex_init(&_mtx, nullptr);
        pthread_cond_init(&_cond, nullptr);
    }
    ThreadPool(const ThreadPool&)
    {}
public:
    static ThreadPool* GetInstance()
    {
        static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
        if (_tp == nullptr)
        {
            pthread_mutex_lock(&mtx);
            if (_tp == nullptr)
            {
                _tp = new ThreadPool();
                _tp->InitThreadPool();
            }
            pthread_mutex_unlock(&mtx);
        }
        return _tp;
    }
    void Wait()
    {
        pthread_cond_wait(&_cond, &_mtx);
    }
    void WakeUp()
    {
        pthread_cond_signal(&_cond);
    }
    void Lock()
    {
        pthread_mutex_lock(&_mtx);
    }
    void UnLock()
    {
        pthread_mutex_unlock(&_mtx);
    }
    static void* ThreadRoutine(void* args)
    {
        pthread_detach(pthread_self());
        ThreadPool* tp = (ThreadPool*)args;
        while (true)
        {
            tp->Lock();
            while (tp->_task_queue.empty())
                tp->Wait();
            Task task;
            tp->PopTask(task);
            tp->UnLock();
            task.ProcessOn();
        }
    }
    bool InitThreadPool()
    {
        pthread_t tid;
        for (int i = 0; i < _num; i++)
        {
            if (pthread_create(&tid, nullptr, ThreadRoutine, this) != 0)
            {

                LOG(Fatal, "thread create fail");
                return false;
            }
        }
        LOG(Info, "thread pool init successs");
        return true;
    }
    void PushTask(const Task& task)
    {
        Lock();
        _task_queue.push(task);        
        UnLock();
        WakeUp();
    }
    void PopTask(Task& task)
    {
        task = _task_queue.front();
        _task_queue.pop();
    }
    ~ThreadPool()
    {
        pthread_mutex_destroy(&_mtx);
        pthread_cond_destroy(&_cond);
    }
private:
    int _num;
    std::queue<Task> _task_queue;
    pthread_mutex_t _mtx;
    pthread_cond_t _cond;
    static ThreadPool* _tp;
};

ThreadPool* ThreadPool::_tp = nullptr;