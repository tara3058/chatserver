#定义SRC_LIST变量，包含目录下所有源文件
aux_source_directory(. SRC_LIST)
aux_source_directory(./model MODEL_LIST)
aux_source_directory(./db DB_LIST)
aux_source_directory(./redis REDIS_LIST)

#生成可执行文件server
add_executable(server ${SRC_LIST} ${MODEL_LIST} ${DB_LIST} ${REDIS_LIST})

#server需要链接的文件
target_link_libraries(server muduo_net muduo_base mysqlclient hiredis pthread)