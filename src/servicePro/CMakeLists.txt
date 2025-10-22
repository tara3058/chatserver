aux_source_directory(. SERVICE_BASE_SCR)
#set(SERVICE_BASE_SCR servicebase.cc)

add_library(service_base ${SERVICE_BASE_SCR})

target_link_libraries(service_base muduo_net muduo_base pthread)

target_include_directories(service_base PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR} 
    ${CMAKE_SOURCE_DIR}/include 
    ${CMAKE_SOURCE_DIR}/rpc/include)