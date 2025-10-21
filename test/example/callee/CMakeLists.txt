aux_source_directory(. SRC_LIST)
set(SRC_LIST ${SRC_LIST} ../user.pb.cc)

add_executable(Callee ${SRC_LIST})
target_link_libraries(Callee mprpc protobuf)