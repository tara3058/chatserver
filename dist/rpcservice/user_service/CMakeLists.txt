
include_directories(${PROJECT_SOURCE_DIR}/rpcservice/user_service/include)
include_directories(${PROJECT_SOURCE_DIR}/proto)

include_directories($(CMAKE_CURRENT_BINARY_DIR))

set(ADDITIONAL_SRC 
    ${PROJECT_SOURCE_DIR}/../src/server/model/offlinemsgmodel.cpp
    ${PROJECT_SOURCE_DIR}/../src/server/model/usermodel.cpp
    ${PROJECT_SOURCE_DIR}/../src/server/db/connectionpool.cpp
    ${PROJECT_SOURCE_DIR}/../src/server/db/db.cpp
    ${PROJECT_SOURCE_DIR}/../include/server//model/user.hpp
)


#set(USER_SERVICE_SRC user_service.cc main.cc)
aux_source_directory(. USER_SERVICE_SRC)
add_executable(user_service ${USER_SERVICE_SRC} ${ADDITIONAL_SRC})
target_link_libraries(user_service proto_c service_base mprpc mysqlclient)