cmake_minimum_required(VERSION 3.10)

# 设置项目名称
project(ConnectionPoolTest)

# 设置C++标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置包含目录
include_directories(../include)
include_directories(../include/server/db)

# 添加源文件
aux_source_directory(../src/server/db DB_SOURCES)

# 创建测试可执行文件
add_executable(test_connectionpool 
    test_connectionpool.cpp
    ${DB_SOURCES}
)

# 链接库
target_link_libraries(test_connectionpool 
    mysqlclient 
    pthread
    muduo_base
)