#pragma once

#include <iostream>
#include "Protocol.hpp"

class Task
{
public:
    Task()
    {}
    Task(int sock)
        :_sock(sock)
    {}
    void ProcessOn()
    {
        _handle(_sock);
    }
    ~Task()
    {}
private:
    int _sock;
    CallBack _handle;
};