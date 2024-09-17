# 最低版本号
cmake_minimum_required(VERSION 2.8)

# 项目名称
project(ConcurrentMemoryPool)

# 生成可执行文件
add_executable(Benchmark Benchmark.cpp)
# add_executable(UnitTest UnitTest.cpp)

# 链接pthread库
target_link_libraries(Benchmark pthread)
# target_link_libraries(UnitTest pthread)

# 设置构建类型
set(CMAKE_BUILD_TYPE "Debug")
set(CMAKE_CXX_STANDARD 11) #支持C++11
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) #闭编译器的 GNU 扩展，以确保代码更符合标准 C++