aux_source_directory(. PROTOC_SCR)

add_library(proto_c ${PROTOC_SCR})

target_link_libraries(proto_c protobuf)

target_include_directories(proto_c PUBLIC ${CMAKE_CURRENT_SOURCE_DIR} )
