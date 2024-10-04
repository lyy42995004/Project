#pragma once

#include <signal.h>
#include "TcpServer.hpp"
#include "Task.hpp"
#include "ThreadPool.hpp"
#include "Log.hpp"

class HttpServer
{
public:
    HttpServer(int port)
        :_port(port)
        ,_stop(false)
    {}
    // 初始化服务器
    void InitServer()
    {
        // 忽略SIGPIPE信号，防止写入时崩溃
        signal(SIGPIPE, SIG_IGN);
    }
    // 启动服务器
    void Loop()
    {
        LOG(Info, "Loop begin");
        int listen_sock = TcpServer::GetInstance(_port)->Sock();
        while (!_stop)
        {
            struct sockaddr_in peer;
            socklen_t len = sizeof(peer);

            int sock = accept(listen_sock, (sockaddr*)&peer, &len);    
            if (sock < 0)
            {
                LOG(Warning, "accept fail");
                continue;
            }
            LOG(Info, "get a new link");

            Task task(sock);
            ThreadPool::GetInstance()->PushTask(task); 
        }
    }
private:
    int _port;
    bool _stop;
};