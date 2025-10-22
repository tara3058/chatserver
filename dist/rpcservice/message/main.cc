#include "message.h"
#include "mprpcapplication.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>

// 全局服务指针，用于信号处理
MessageServiceImpl* g_messageService = nullptr;

// 信号处理函数
void sigHandler(int sig)
{
    if (g_messageService) {
        g_messageService->Stop();
    }
}

int main(int argc, char** argv)
{
    // 检查命令行参数
    if (argc < 3) {
        std::cout << "Usage: " << argv[0] << " serverIP serverPort" << std::endl;
        std::cout << "Example: " << argv[0] << " 127.0.0.1 8082" << std::endl;
        return -1;
    }
    
    // 获取IP和端口
    std::string ip = argv[1];
    uint16_t port = static_cast<uint16_t>(std::stoi(argv[2]));
    
    // 注册信号处理函数
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
    
    // 构造新的命令行参数，包含配置文件
    char* new_argv[argc + 3];
    new_argv[0] = argv[0];
    new_argv[1] = const_cast<char*>("-i");
    new_argv[2] = const_cast<char*>("../mprpc.conf");
    
    // 复制原始参数
    for (int i = 3; i < argc + 3; ++i) {
        new_argv[i] = argv[i - 2];
    }
    
    // 初始化RPC框架
    MprpcApplication::Init(argc + 2, new_argv);
    
    // 创建消息服务
    MessageServiceImpl messageService(ip, port);
    g_messageService = &messageService;
    
    // 启动服务
    messageService.Start();
    
    return 0;
}
