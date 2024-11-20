#pragma once

#include <iostream>

#define RED "\033[31m"
#define COL_END "\033[0m"

enum
{
    Info = 0,
    Debug = 1,
    Warning = 2,
    Error = 3,
    Fatal = 4
};

// 定义宏函数，#level 将level转化为字符
#define LOG(level, message) Log(#level, message, __FILE__, __LINE__)

// 简单实现日志函数
void Log(std::string level, std::string message, std::string file_name, int line)
{
    // std::string _level = RED;
    // _level += level;
    // _level += COL_END;
    std::cout << "[" << level << "][" << time(nullptr) << "][" << message 
              << "][" << file_name << "][" << line << "]" << std::endl;
}