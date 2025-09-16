#ifndef CHATSERVER_H
#define CHATSERVER_H

#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>

using namespace muduo;
using namespace muduo::net;
class ChatServer{
public: 
    //初始化
    ChatServer(EventLoop* loop, //循环
            const InetAddress& listenAddr, //IP+Port
            const string& nameArg); //name

    //启动服务
    void start();
    
private:
    //上报链接相关信息
    void onConnection(const TcpConnectionPtr&);

    //上报读写相关信息
    void onMessage(const TcpConnectionPtr& conn, // 链接
                    Buffer* buffer, //缓冲区
                    Timestamp time);// 接受到数据的时间

    TcpServer _server;
    EventLoop* _loop;

};

#endif