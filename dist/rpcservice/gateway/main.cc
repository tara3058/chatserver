// dist/rpcservice/gateway/main.cc
#include "gateway.h"
#include "mprpcapplication.h"
#include <muduo/base/Logging.h>

int main(int argc, char** argv)
{
    // 初始化RPC框架
    MprpcApplication::Init(argc, argv);
    
    // 设置日志级别
    muduo::Logger::setLogLevel(muduo::Logger::DEBUG);
    
    // 创建并启动网关服务
    GatewayService gatewayService("127.0.0.1", 8080);
    gatewayService.Start();
    
    return 0;
}