#需要的最低版本
cmake_minimum_required(VERSION 3.0)
#工程的名字
project(main)

#配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

#头文件搜索
#include_directories()
#库文件搜索
#link_directories()

#设置需要编译的源文件列表
set(SRC_LIST testmoduo.cpp)
#生成可执行文件server
add_executable(sever ${SRC_LIST})
#server需要链接的文件
target_link_libraries(sever muduo_net muduo_base pthread)
