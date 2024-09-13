# 最低版本号
cmake_minimum_required(VERSION 2.8)
# 项目名称
project(ConcurrentMemoryPool)
# 生成可执行文件
add_executable(UnitTest UnitTest.cpp)
# 链接pthread库
target_link_libraries(UnitTest pthread)
# 设置构建类型
set(CMAKE_BUILD_TYPE "Debug")