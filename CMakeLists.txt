#需要的最低版本
cmake_minimum_required(VERSION 3.10)
#工程的名字
project(chatServer)

#配置编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")



#配置最终可执行文件输出的路径
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
#配置最终库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

#头文件搜索
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/include/server)
include_directories(${PROJECT_SOURCE_DIR}/include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/include/server/redis)
include_directories(${PROJECT_SOURCE_DIR}/include/serverpro)
include_directories(${PROJECT_SOURCE_DIR}/thirdparty)
include_directories(${PROJECT_SOURCE_DIR}/rpc/include)
include_directories(${PROJECT_SOURCE_DIR}/rpc/include/mprpclog)
#include_directories(${PROJECT_SOURCE_DIR}/dist)
#include_directories(${PROJECT_SOURCE_DIR}/dist/proto)
#库文件搜索
link_directories(${PROJECT_SOURCE_DIR}/lib)

#加载子目录
add_subdirectory(src)
add_subdirectory(rpc)
add_subdirectory(dist)





