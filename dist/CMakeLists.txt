cmake_minimum_required(VERSION 3.10)
project(Dist_Chat)

#set(PROTOBUF_INCLUDE_DIRS "/usr/local/protobuf/include")
#set(PROTOBUF_LIBRARIES "/usr/local/protobuf/lib/libprotobuf.so")
#set(PROJECT_SOURCE_DIR "/usr/local/protobuf/bin/protoc")

include_directories(${PROJECT_SOURCE_DIR}/../rpc/include)
include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/../include/server)
include_directories(${PROJECT_SOURCE_DIR}/../include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/../include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/proto)
include_directories(${PROJECT_SOURCE_DIR}/rpcservice)
include_directories(${PROJECT_SOURCE_DIR}/rpcservice/include)
include_directories(${PROTOBUF_INCLUDE_DIRS})

link_directories(${PROJECT_SOURCE_DIR}/../lib)

add_subdirectory(proto)
add_subdirectory(rpcservice/user_service)
add_subdirectory(rpcservice/message)
add_subdirectory(rpcservice/relation_service)
add_subdirectory(rpcservice/gateway)