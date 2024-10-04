#pragma once

#include "TcpServer.hpp"
// #include "Protocol.hpp"
#include "Protocol.hpp"
#include "Log.hpp"

class HttpServer
{
public:
    HttpServer(int port)
        :_port(port)
        ,_running(true)
        ,_tcp_server(nullptr)
    {}
    // 初始化服务器
    void InitServer()
    {
        _tcp_server = TcpServer::GetInstance(_port);
    }
    // 启动服务器
    void Loop()
    {
        LOG(Info, "Loop begin");
        int listen_sock = _tcp_server->Sock();
        while (_running)
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
            int* psock = new int(sock);
            pthread_t tid;
            pthread_create(&tid, nullptr, Entrance::HandleRequest, psock);
            pthread_detach(tid);
        }
    }
private:
    int _port;
    bool _running;
    TcpServer* _tcp_server;
};