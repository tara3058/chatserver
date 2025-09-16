#include<iostream>
#include"chatserver.hpp"
#include"chatservice.hpp"
#include<signal.h>
#include<muduo/net/EventLoop.h>
#include<muduo/net/InetAddress.h>

using namespace std;
using namespace muduo;
using namespace muduo::net;

// 处理服务器ctrl+c结束后，重置user的状态信息
void resetHandler(int)
{
    Chatservice::instance()->reset();
    exit(0);
}

int main(int argc, char **argv)
{
    if(argc < 3)
    {
        cerr << "command invalid! example: ./ChatServer 127.0.0.1 6000" << endl;
        exit(-1);
    }

    char *ip = argv[1];
    uint16_t port = atoi(argv[2]);
    
    signal(SIGINT, resetHandler);

    EventLoop loop;
    InetAddress addr(ip, port);
    ChatServer server(&loop, addr, "Chatserver");

    server.start();
    loop.loop();

    return 0;
}