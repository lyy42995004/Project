#include <iostream>
#include <string>
#include <cstdlib>
#include <unistd.h>

//
static bool GetQueryString(std::string& query_string)
{
    std::string method = getenv("METHOD");
    // std::cerr << "for test: method = " << method << std::endl;
    if (method == "GET")
    {
        query_string =  getenv("QUERY_STRING");
        // std::cerr << "for test: query string= " << query_string << std::endl;
        return true;
    }
    else if (method == "POST")
    {
        int content_length = atoi(getenv("CONTENT_LENGTH"));
        char ch = 0;
        while (content_length)
        {
            read(0, &ch, 1); // 从管道中读取数据
            query_string.push_back(ch);
            content_length--;
        }
        return true;
        // std::cerr << "for test: query string= " << query_string << std::endl;
    }
    else
        return false;
}

// 将字符串切割成两部分
static bool CutString(const std::string& target, const std::string sep, std::string& lhs, std::string& rhs)
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

int main()
{
    std::string query_string;
    GetQueryString(query_string);

    // a=100&b=200
    // 以&为分隔符将两个操作数分开
    std::string str1;
    std::string str2;
    CutString(query_string, "&", str1, str2);

    // 以=为分隔符分别获取两个操作数的值
    std::string name1;
    std::string value1;
    CutString(str1, "=", name1, value1);
    std::string name2;
    std::string value2;
    CutString(str2, "=", name2, value2);

    std::cout << name1 << "=" << value1 << std::endl;
    std::cout << name2 << "=" << value2 << std::endl;

    std::cerr << name1 << "=" << value1 << std::endl;
    std::cerr << name2 << "=" << value2 << std::endl;

    return 0;
}