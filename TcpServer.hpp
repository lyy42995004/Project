#pragma once

#include <cstdlib>
#include <cstring>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "Log.hpp"

#define BACKLOG 10

class TcpServer
{
private:
    TcpServer(int port)
        :_port(port)
        ,_listen_sock(-1)
    {}
    TcpServer(const TcpServer&) = delete;
public:
    static TcpServer* GetInstance(int port)
    {
        static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
        if (_svr == nullptr)
        {
            pthread_mutex_lock(&mtx);
            if (_svr == nullptr)
            {
                _svr = new TcpServer(port);
                _svr->InitServer();
            }
            pthread_mutex_unlock(&mtx);
        }
        return _svr;
    }
    // 初始化
    void InitServer()
    {
        Socket();
        Bind();
        Listen();
        LOG(Info, "tcp_server init success");
    }
    // 创建套接字
    void Socket()
    {
        _listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (_listen_sock < 0)
        {
            LOG(Fatal, "socket fail");
            exit(1);
        }
        int opt = 1;
        if(setsockopt(_listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
            LOG(Warning, "setsockopt fail");
        LOG(Info, "create socket success");
    }
    // 绑定
    void Bind()
    {
        struct sockaddr_in local;
        memset(&local, 0, sizeof(local));
        local.sin_family = AF_INET;
        local.sin_port = htons(_port);
        local.sin_addr.s_addr = INADDR_ANY;
        if (bind(_listen_sock, (sockaddr*)&local, sizeof(local)) < 0)
        {
            LOG(Fatal, "bind fail");
            exit(1);
        }
        LOG(Info, "bind socket success");
    }
    // 监听
    void Listen()
    {
        if (listen(_listen_sock, BACKLOG) < 0)
        {
            LOG(Fatal, "listen fail");
            exit(1);
        }
        LOG(Info, "listen socket success");
    }
    int Sock()
    {
        return _listen_sock;
    }
    ~TcpServer()
    {
        if (_listen_sock >= 0)
            close(_listen_sock);
    }

private:
    int _port;
    int _listen_sock;
    static TcpServer* _svr;
};

TcpServer* TcpServer::_svr = nullptr;