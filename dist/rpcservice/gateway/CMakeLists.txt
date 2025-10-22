# dist/rpcservice/gateway/CMakeLists.txt
aux_source_directory(. GATEWAY_SOURCES)

# 包含头文件目录
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/src/servicePro)
include_directories(${CMAKE_SOURCE_DIR}/dist/proto)
include_directories(${CMAKE_SOURCE_DIR}/rpc/include)
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/include/server)

add_executable(gateway_service ${GATEWAY_SOURCES})

target_link_libraries(gateway_service 
    service_base
    proto_c
    muduo_net 
    muduo_base 
    pthread
    mprpc
    zookeeper_mt
)

# 设置C++标准
#target_compile_features(gateway_service PRIVATE cxx_std_14)