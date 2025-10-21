#include "include/usr_service.h"
#include "mprpcapplication.h"
#include <muduo/base/Logging.h>

int main(int  argc, char **argv){
    
    MprpcApplication::Init(argc, argv);

    UserServiceImpl userservice("127.0.0.1", 8081);
    userservice.Start();
}