aux_source_directory(. SRC_LIST)

#set(SRC_LIST ${MPRPC_SRC_LIST} ./mprpclog)

add_library(mprpc ${SRC_LIST})
#add the_definitions(-DTHREADED)
target_compile_definitions(mprpc PRIVATE THREADED)
target_link_libraries(mprpc muduo_net muduo_base pthread zookeeper_mt)
