#pragma once

#include <iostream>
#include <string>
#include <sys/types.h>
#include <sys/socket.h>

// 工具类
class Util
{
public:
    // 读取一行数据
    static int Readline(int sock, std::string& out)
    {
        char ch = '1';
        while (ch != '\n')
        {
            ssize_t size = recv(sock, &ch, 1, 0);
            if (size > 0)
            {
                // 将 \r \n \r\n 三种情况都转化为 \n
                if (ch == '\r')
                {
                    recv(sock, &ch, 1, MSG_PEEK);
                    if (ch == '\n')
                        recv(sock, &ch, 1, 0);
                    else
                        ch = '\n';
                }
                out.push_back(ch);
            }
            else if (size == 0)
                return 0;
            else
                return -1;
        }
        return out.size();
    }
    // 将字符串切割成两部分
    static bool CutString(const std::string& target, std::string& lhs, std::string& rhs, const std::string sep)
    {
        size_t pos = target.find(sep);
        if (pos != std::string::npos)
        {
            lhs = target.substr(0, pos);
            rhs = target.substr(pos + sep.size());
            return true;
        }
        return false;
    }
};