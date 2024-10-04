#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include "Util.hpp"
#include "Log.hpp"

#define SEP ": "
#define WEB_ROOT "wwwroot"
#define HOME_PAGE "index.html"
#define PAGE_404 "404.html"
#define HTTP_VERSION "HTTP/1.0"
#define LINE_END "\r\n"

enum
{
    OK = 200,
    BAD_REQUEST = 400,
    NOT_FOUND = 404,
    SERVER_ERROR = 500
};

// 根据状态码获取状态码字符串
static std::string CodeToDesc(int code)
{
    std::string desc;
    switch (code)
    {
    case OK:
        desc = "OK";
        break;
    case BAD_REQUEST:
        desc = "BAD REQUEST";
        break;
    case NOT_FOUND:
        desc = "NOT FOUND"; 
        break;
    case SERVER_ERROR:
        desc = "SERVER ERROR";
        break;
    default:
        break;
    }
    return desc;
}

// 根据后缀获取资源类型
static std::string SuffixToDesc(const std::string& suffix)
{
    static std::unordered_map<std::string, std::string> suffix_to_desc = 
    {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/x-javascript"},
        {".jpg", "application/x-jpg"},
        {".xml", "text/xml"}
    };
    auto iter = suffix_to_desc.find(suffix);
    if (iter != suffix_to_desc.end())
        return iter->second;
    return "text/html"; //所给后缀未找到则默认该资源为html文件
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
    std::string _suffix; // 响应文件的后缀

    HttpResponse()
        :_blank(LINE_END)
        ,_status_code(OK)
        ,_fd(-1)
        ,_size(0)
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
        ,_stop(false)
    {}
    // 判断是否需要停止服务
    bool IsStop()
    {
        return _stop;
    }
    // 读取并解析请求
    void RecvHttpRequest()
    {
        if (!RecvHttpRequstLine() && !RecvHttpRequstHeader())
        {
            ParseHttpRequestLine();
            ParseHttpRequestHeader();
            RecvHttpRequestBody();
        }
    }
    // 处理请求
    void HandleRequest()
    {
        LOG(Info, _http_request._request_line);
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
            code = 404;
            return;
        }

        // 记录响应文件后缀
        size_t pos = path.rfind('.');
        if (pos != std::string::npos)
            _http_response._suffix = path.substr(pos);
        else
            _http_response._suffix = ".html";

        // 判断是否执行CGI模式
        if (cgi)
            code = ProcessCGI();
        else
            code = ProcessNonCGI();
    }
    // 构建响应
    void BuildResponse()
    {
        int code = _http_response._status_code;
        // 构建状态行
        auto& status_line = _http_response._status_line;
        status_line += HTTP_VERSION;
        status_line += " ";
        status_line += std::to_string(code);
        status_line += " ";
        status_line += CodeToDesc(code);
        status_line += LINE_END;
        // 构建响应报头
        std::string path = WEB_ROOT;
        path += '/';
        switch (code)
        {
        case OK:
            BuildOKResponse();
            break;
        case BAD_REQUEST:
            path += PAGE_404;
            BuildErrorResponse(path);
            break;
        case 404:
            path += PAGE_404;
            BuildErrorResponse(path);
            break;
        case SERVER_ERROR:
            path += PAGE_404;
            BuildErrorResponse(path);
            break;
        default:
            break;
        }
    }
    // 发送响应
    void SendResponse()
    {
        send(_sock, _http_response._status_line.c_str(), _http_response._status_line.size(), 0);
        for (auto& iter : _http_response._response_header)
            send(_sock, iter.c_str(), iter.size(), 0);
        send(_sock, _http_response._blank.c_str(), _http_response._blank.size(), 0);
        if (_http_request._cgi) // cgi
        {
            auto& response_body = _http_response._response_body;
            const char* start = response_body.c_str();
            int total = 0;
            ssize_t size = 0;
            // 一直写入直到写完
            while ((total < response_body.size()) 
                && (size = write(_sock, start + total, response_body.size() - total)))
                total += size;
        }
        else // 非cgi
        {
            sendfile(_sock, _http_response._fd, nullptr, _http_response._size);
            close(_http_response._fd);
        }
    }
    ~EndPoint()
    {
        close(_sock);
    }
private:
    // 读取请求行
    bool RecvHttpRequstLine()
    {
        auto& line = _http_request._request_line;
        if (Util::ReadLine(_sock, line) <= 0)
        {
            _stop = true;
            return _stop;
        }
        line.resize(line.size()-1);
    }
    // 读取请求报头
    bool RecvHttpRequstHeader()
    {
        std::string line;
        while (true)
        {
            line.clear();
            if (Util::ReadLine(_sock, line) <= 0)
            {
                _stop = true;
                return _stop;
            }
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

        // 将请求方法转化为大写
        auto& method = _http_request._method;
        std::transform(method.begin(), method.end(), method.begin(), toupper);
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
    bool RecvHttpRequestBody()
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
                {
                    _stop = true;
                    return _stop;
                }
            }
        }
    }
    // 非CGI处理
    int ProcessNonCGI()
    {
        _http_response._fd = open(_http_request._path.c_str(), O_RDONLY);
        if (_http_response._fd >= 0)
            return OK;
        return 404;
    }
    // CGI处理，获得_response_body
    int ProcessCGI()
    {
        LOG(Info, "process cgi");
        // 创建管道用于进程间通信
        int input[2];
        int output[2];
        if (pipe(input) < 0)
        {
            LOG(Error, "pipe input fail");
            return SERVER_ERROR;
        }
        if (pipe(output) < 0)
        {
            LOG(Error, "pipe output fail");
            return SERVER_ERROR;
        }

        auto& bin = _http_request._path;
        auto& method = _http_request._method;
        auto& query_string = _http_request._query_string;
        auto& request_body = _http_request._request_body;
        pid_t pid = fork();
        if (pid == 0) // child
        {
            // 子进程关闭两个管道对应的读写端
            close(input[0]);
            close(output[1]);

            // 将请求方法添加到环境变量
            std::string method_env = "METHOD=";
            method_env += _http_request._method;
            putenv((char*)method_env.c_str());
            // GET方法时，将query_string添加到环境变量
            if (method == "GET")
            {
                std::string query_string_env = "QUERY_STRING=";
                query_string_env += query_string;
                putenv((char*)query_string_env.c_str());
            }
            // POST方法时，将content_length添加到环境变量
            if (method == "POST")
            {
                std::string content_length_env = "CONTENT_LENGTH=";
                content_length_env += std::to_string(_http_request._content_length);
                putenv((char*)content_length_env.c_str());
            }

            // 重定向文件描述符
            // input[1]  写数据 -> 1
            // output[0] 读数据 -> 0
            dup2(input[1], 1);
            dup2(output[0], 0);

            // 将子进程替换成CGI程序
            execl(bin.c_str(), bin.c_str(), nullptr);
            exit(1);
        }
        else if (pid > 0) // parent
        {
            // 父进程关闭两个管道对应的读写端
            close(input[1]);
            close(output[0]);
            // POST方法时，向管道写入请求内容
            if (method == "POST")
            {
                const char* start = request_body.c_str();
                int total = 0;
                ssize_t size = 0;
                // 一直写入直到写完
                while ((total < _http_request._content_length) 
                    && (size = write(output[1], start + total, request_body.size() - total)))
                    total += size;
                if (size < 0)
                    LOG(Error, "write request body fail");
            }

            // 读取cgi程序向管道中写入的数据
            auto& response_body = _http_response._response_body;
            char ch = 0;
            while (read(input[0], &ch, 1) > 0)
                response_body.push_back(ch);

            // 等待子进程退出
            int status = 0;
            pid_t ret = waitpid(pid, &status, 0);
            if (ret == pid)
                if (WIFEXITED(status)) // 判断进程是否正常退出
                    if (WEXITSTATUS(status) == 0) // 判断进程退出码 
                        return OK;
                    else
                        return BAD_REQUEST;
                else
                    return SERVER_ERROR;

            // 关闭文件描述符
            close(input[0]);
            close(input[1]);
        }
        else
        {
            LOG(Error, "fork fail");
            return SERVER_ERROR;
        }
        return OK;
    }
    // 构建OK对应响应报头
    void BuildOKResponse()
    {
        std::string head_line = "Content-Type: ";
        head_line += SuffixToDesc(_http_response._suffix);
        head_line += LINE_END;
        _http_response._response_header.push_back(head_line);
        head_line = "Content-Length: ";
        if (_http_request._cgi) // cgi
            head_line += std::to_string(_http_response._response_body.size());
        else
            head_line += std::to_string(_http_response._size);
        head_line += LINE_END;
        _http_response._response_header.push_back(head_line);
    }
    // 出错时，构建对应响应报头
    void BuildErrorResponse(std::string path)
    {
        _http_request._cgi = false;
        _http_response._fd = open(path.c_str(), O_RDONLY);
        if (_http_response._fd > 0)
        {
            struct stat st;
            stat(path.c_str(), &st);
            _http_response._size = st.st_size;
            std::string line = "Content-Type: text/html";
            line += LINE_END;
            _http_response._response_header.push_back(line);

            line = "Content-Length: ";
            line += std::to_string(st.st_size);
            line += LINE_END;
            _http_response._response_header.push_back(line);
        }
        else
            LOG(Error, "open fail");
    }

private:
    int _sock;
    bool _stop;
    HttpRequest _http_request;
    HttpResponse _http_response;
};

class CallBack
{
public:
    CallBack()
    {}
    void operator()(int sock)
    {
        HandleRequest(sock);
    }
    void HandleRequest(int sock)
    {
        LOG(Info, "handle request begin");

        // // for test
        // std::cout << "get a new link ... " << sock << std::endl;
        // char buffer[4096];
        // recv(sock, buffer, sizeof(buffer), 0);
        // std::cout << "--------------begin--------------" << std::endl;
        // std::cout << buffer << std::endl;
        // std::cout << "---------------end---------------" << std::endl;

        EndPoint* ep = new EndPoint(sock);
        ep->RecvHttpRequest();
        if (!ep->IsStop())
        {
            ep->HandleRequest();
            ep->BuildResponse();
            ep->SendResponse();
        }
        else
            LOG(Warning, "recv error, stop server");
        delete ep;
        LOG(Info, "handle request end");
    }
    ~CallBack()
    {}
};