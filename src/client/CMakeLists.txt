


#定义SRC_LIST变量，包含目录下所有源文件
aux_source_directory(. SRC_LIST)
#生成可执行文件server
add_executable(client ${SRC_LIST})
#clinet需要链接的文件
target_link_libraries(client pthread)