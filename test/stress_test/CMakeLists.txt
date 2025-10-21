cmake_minimum_required(VERSION 3.0)
project(ChatStressTest)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -std=c++14")
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/../../bin)

# 头文件搜索路径
include_directories(${PROJECT_SOURCE_DIR}/../../include)
include_directories(${PROJECT_SOURCE_DIR}/../../include/server)
include_directories(${PROJECT_SOURCE_DIR}/../../thirdparty)

# 查找muduo库
find_library(MUDUO_BASE muduo_base)
find_library(MUDUO_NET muduo_net)
find_library(PTHREAD pthread)

# 检查muduo库是否找到
if(NOT MUDUO_BASE)
    message(FATAL_ERROR "muduo_base库未找到，请安装muduo库")
endif()

if(NOT MUDUO_NET)
    message(FATAL_ERROR "muduo_net库未找到，请安装muduo库")
endif()

# 创建压测客户端可执行文件
add_executable(stress_client stress_client.cpp)

# 链接库
target_link_libraries(stress_client 
    ${MUDUO_BASE} 
    ${MUDUO_NET} 
    ${PTHREAD}
)

# 创建简单的连接测试工具
add_executable(connection_test connection_test.cpp)
target_link_libraries(connection_test 
    ${MUDUO_BASE} 
    ${MUDUO_NET} 
    ${PTHREAD}
)