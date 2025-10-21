#include"userservice.hpp"
#include"mprpcapplication.h"

int main(int argc, char **argv)
{
    //框架初始化
    MprpcApplication::Init(argc, argv);

    //把userService对象发布到RPC节点
    MprpcProvider provider;
    provider.NotifyService(new Userserive());

    // 启动一个rpc服务发布节点   Run以后，进程进入阻塞状态，等待远程的rpc调用请求
    provider.StartMprpc();
    
    return 0;
}