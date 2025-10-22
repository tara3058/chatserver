# 消息服务的CMakeLists.txt
set(SERVICE_NAME message_service)

# 包含目录
include_directories(${PROJECT_SOURCE_DIR}/../rpc/include)
include_directories(${PROJECT_SOURCE_DIR}/../include)
include_directories(${PROJECT_SOURCE_DIR}/../include/server)
include_directories(${PROJECT_SOURCE_DIR}/../include/server/db)
include_directories(${PROJECT_SOURCE_DIR}/../include/server/model)
include_directories(${PROJECT_SOURCE_DIR}/../../dist/proto)
include_directories(${PROJECT_SOURCE_DIR}/../rpcservice)
include_directories(${PROJECT_SOURCE_DIR}/include)

# 源文件
set(SRC_LIST 
    message.cc
    main.cc
    ${PROJECT_SOURCE_DIR}/../src/server/model/offlinemsgmodel.cpp
    ${PROJECT_SOURCE_DIR}/../src/server/db/connectionpool.cpp
    ${PROJECT_SOURCE_DIR}/../src/server/db/db.cpp
)

# 添加可执行文件
add_executable(${SERVICE_NAME} ${SRC_LIST})

# 链接库
target_link_libraries(${SERVICE_NAME} 
    service_base
    proto_c
    mprpc
    muduo_net
    muduo_base
    zookeeper_mt
    mysqlclient
    pthread
)