#include <functional>
#include <string>
#include "chatserver.hpp"
#include "json.hpp"
#include"chatservice.hpp"

using namespace std;
using namespace placeholders;
using json = nlohmann::json;

ChatServer::ChatServer(EventLoop *loop,               // 循环
                       const InetAddress &listenAddr, // IP+Port
                       const string &nameArg)
    : _server(loop, listenAddr, nameArg), _loop(loop)
{
    // 注册链接创建断开回调
    _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
    // 注册读写事件回调
    _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
    // 设置服务器线程数
    _server.setThreadNum(4);
}

////启动服务
void ChatServer::start(){
    _server.start();
}

// 上报链接相关信息
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        Chatservice::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}

// 上报读写相关信息
void ChatServer::onMessage(const TcpConnectionPtr &conn,
                           Buffer *buffer,
                           Timestamp time)
{
    // 接收缓冲区数据
    string buf = buffer->retrieveAllAsString();
    // 数据反序列化
    json js = json::parse(buf);
    //通过js[msg_id]获得业务hanlder
    auto msghandler = Chatservice::instance()->getHandler(js["msgid"].get<int>());
    msghandler(conn, js,time);

}
