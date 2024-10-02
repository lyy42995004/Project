#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"

enum
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404
};

static std::string CodeToDesc(int code)
{
    std::string desc;
    switch (code)
    {
    case OK:
        desc = "OK";
        break;
    case BAD_REQUEST:
        desc = "BAD_REQUEST";
        break;
    case NOT_FOUND:
        desc = "NOT FOUND"; 
        break;
    default:
        break;
    }
    return desc;
}

class HttpRequest
{
public:
    std::string _request_line;                // 请求行
    std::vector<std::string> _request_header; // 请求报头
    std::string _blank;                       // 空行 
    std::string _request_body;                // 请求正文
    // 解析结果
    std::string _method;       // 请求方法
    std::string _uri;          // URI
    std::string _version;      // 版本号
    std::unordered_map<std::string, std::string> _header_kv; // 请求报头中的键值对
    int _content_length;       // 正文长度
    std::string _path;         // 请求资源的路径
    std::string _query_string; // uri中携带的参数

    bool _cgi; // 是否需要使用CGI模式

    HttpRequest()
        :_content_length(0)
        ,_cgi(false)
    {}
    ~HttpRequest()
    {}
};

class HttpResponse
{
public:
    std::string _status_line;                  // 状态行
    std::vector<std::string> _response_header; // 响应报头
    std::string _blank;                        // 空行 
    std::string _response_body;                // 响应正文

    int _status_code;    // 状态码
    int _fd;             // 响应文件的fd
    int _size;           // 响应文件的大小

    HttpResponse()
        :_blank(LINE_END)
        ,_status_code(OK)
        ,_fd(-1)
    {}
    ~HttpResponse()
    {}
};

// 服务器端EndPoint
class EndPoint
{
public:
    EndPoint(int sock)
        :_sock(sock)
    {}
    // 读取并解析请求
    void RecvHttpRequest()
    {
        RecvHttpRequstLine();
        RecvHttpRequstHeader();
        ParseHttpRequestLine();
        ParseHttpRequestHeader();
        RecvHttpRequestBody();
    }
    // 构建响应
    void BuildResponse()
    {
        auto& code = _http_response._status_code;
        auto& path = _http_request._path;
        auto& cgi = _http_request._cgi;
        // 处理GET请求
        if (_http_request._method == "GET")
        {
            size_t pos = _http_request._uri.find('?');
            if (pos != std::string::npos)
            {
                Util::CutString(_http_request._uri, path, _http_request._query_string, "?");
                cgi = true; // 上传参数，需要使用CGI模式
            }
            else
                path = _http_request._uri;
        }
        else if (_http_request._method == "POST")
        {
            path = _http_request._uri;
            cgi = true;
        }
        else
        {
            LOG(Warning, "unable to handle this request");
            code = BAD_REQUEST;
            return;
        }
        // 给请求资源拼接Web根目录
        path = WEB_ROOT + path;
        // 请求资源以'/'为结尾说明是个目录，加上index.html
        if (path[path.size() - 1] == '/')
            path += HOME_PAGE;

        // 获取请求文件属性信息
        struct stat st;
        if (stat(path.c_str(), &st) == 0) // 属性信息获取成功，说明资源存在
        {
            if (S_ISDIR(st.st_mode)) // 判断是否是目录
            {
                // 拼接上 /index.html
                path += "/";
                path += HOME_PAGE;
                stat(path.c_str(), &st); // 重新获取属性信息
            }
            else if ((st.st_mode & S_IXUSR) || (st.st_mode & S_IXGRP) || (st.st_mode & S_IXOTH)) // 判断是否是可执行文件
                cgi = true;
            _http_response._size = st.st_size; // 记录文件大小
        }
        else
        {
            LOG(Warning, path + " NOT FOUND");
            code = NOT_FOUND;
            return;
        }
        if (cgi)
        {
            ProcessCGI();
        }
        else
        {
            ProcessNonCGI();
        }
    }
    // 发送响应
    void SendResponse()
    {
        send(_sock, _http_response._status_line.c_str(), _http_response._status_line.size(), 0);
        for (auto& iter : _http_response._response_header)
            send(_sock, iter.c_str(), iter.size(), 0);
        send(_sock, _http_response._blank.c_str(), _http_response._blank.size(), 0);
        sendfile(_sock, _http_response._fd, nullptr, _http_response._size);
        close(_http_response._fd);
    }
    ~EndPoint()
    {
        close(_sock);
    }
private:
    // 读取请求行
    void RecvHttpRequstLine()
    {
        auto& line = _http_request._request_line;
        Util::Readline(_sock, line);
        line.resize(line.size()-1);
    }
    // 读取请求报头
    void RecvHttpRequstHeader()
    {
        std::string line;
        while (true)
        {
            line.clear();
            Util::Readline(_sock, line);
            if (line == "\n")
            {
                _http_request._blank = line;
                break; 
            }
            line.resize(line.size() - 1);
            _http_request._request_header.push_back(line);
        }
    }
    // 解析请求行
    void ParseHttpRequestLine()
    {
        auto& line = _http_request._request_line;
        std::stringstream ss(line);
        ss >> _http_request._method >> _http_request._uri >> _http_request._version;
    }
    // 解析请求报头
    void ParseHttpRequestHeader()
    {
        std::string key;
        std::string value;
        for (auto& iter : _http_request._request_header)
        {
            if (Util::CutString(iter, key, value, SEP))
                _http_request._header_kv.insert({key, value});
            else
                LOG(Error, "parse http_request header fail");
        }
    }
    // 判断是否需要读取请求正文
    bool IsNeedRecvHttpRequestBody()
    {
        auto& method = _http_request._method;
        if (method == "POST") // 请求方法为POST时需要读取正文
        {
            auto& header_kv = _http_request._header_kv;
            auto iter = header_kv.find("Content-Length");
            if (iter != header_kv.end())
            {
                _http_request._content_length = atoi(iter->second.c_str()); 
                return true;
            }
        }
        return false;
    }
    // 读取请求正文
    void RecvHttpRequestBody()
    {
        if (IsNeedRecvHttpRequestBody())
        {
            char ch = 0;
            int content_length = _http_request._content_length;
            auto& body = _http_request._request_body;
            while (content_length)
            {
                ssize_t size = recv(_sock, &ch, 1, 0);
                if (size > 0)
                {
                    body.push_back(ch);
                    content_length--;
                }
                else
                    break;
            }
        }
    }
    void ProcessCGI()
    {
    }
    // 返回简单的静态网页
    void ProcessNonCGI()
    {
        _http_response._fd = open(_http_request._path.c_str(), O_RDONLY);
        if (_http_response._fd >= 0)
        {
            auto& status_line = _http_response._status_line;
            status_line = HTTP_VERSION;
            status_line += " ";
            status_line += std::to_string(_http_response._status_code);
            status_line += " ";
            status_line += CodeToDesc(_http_response._status_code);
            status_line += LINE_END;
        }
    }
private:
    int _sock;
    HttpRequest _http_request;
    HttpResponse _http_response;
};

class Entrance
{
public:
    static void* HandleRequset(void* psock)
    {
        LOG(Info, "handle request begin");
        int sock = *(int*)psock;
        delete (int*)psock;

        // // for test
        // std::cout << "get a new link ... " << sock << std::endl;
        // char buffer[4096];
        // recv(sock, buffer, sizeof(buffer), 0);
        // std::cout << "--------------begin--------------" << std::endl;
        // std::cout << buffer << std::endl;
        // std::cout << "---------------end---------------" << std::endl;

        EndPoint* ep = new EndPoint(sock);
        ep->RecvHttpRequest();
        ep->BuildResponse();
        ep->SendResponse();
        delete ep;
        LOG(Info, "handle request end");

        return nullptr;
    }
};