#include <iostream>
#include <memory>
#include "HttpServer.hpp"
#include "Log.hpp"

int main(int argc, char* argv[])
{
    if (argc != 2)
    {
        std::cout <<"Usage: ./httpserver [port]" << std::endl;
        return 0;
    }
    int port = atoi(argv[1]);
    std::shared_ptr<HttpServer> http_server(new HttpServer(port));
    http_server->InitServer();
    http_server->Loop();
    return 0;
}