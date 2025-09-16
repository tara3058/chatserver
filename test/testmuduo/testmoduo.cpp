#include<muduo/net/TcpServer.h>
#include<muduo/net/EventLoop.h>
#include<iostream>
#include<functional>
#include<string>
using namespace std;
using namespace muduo;
using namespace muduo::net;
using namespace placeholders;//占位符

class ChatServer{
public: 
    ChatServer(EventLoop* loop, //循环
            const InetAddress& listenAddr, //IP+Port
            const string& nameArg) //name
            :_server(loop, listenAddr, nameArg)
            ,_loop(loop)
            {
                //注册链接创建断开回调
                _server.setConnectionCallback(std::bind(&ChatServer::onConnection, this, _1));
                //注册读写事件回调
                _server.setMessageCallback(std::bind(&ChatServer::onMessage, this, _1, _2, _3));
                //设置服务器线程数
                _server.setThreadNum(4);
            }

            void start(){
                _server.start();
            }
    
private:
    //处理用户链接创建和断开 epoll listen accept
    void onConnection(const TcpConnectionPtr& conn){
        if(conn->connected()){
            cout << conn->peerAddress().toIpPort() << "->" << 
            conn->localAddress().toIpPort() << "state: online" << endl;
        }
        else{
            cout << conn->peerAddress().toIpPort() << "->" << 
            conn->localAddress().toIpPort() << "state: offline" << endl;
            conn->shutdown();
        }
    }
    //处理用户读写事件
    void onMessage(const TcpConnectionPtr& conn, // 链接
                            Buffer* buffer, //缓冲区
                            Timestamp time)// 接受到数据的时间
    {
        string buf = buffer->retrieveAllAsString();
        cout<<"recv data" << buf << "time" << time.toString() << endl;
        conn->send(buf);
    }
    TcpServer _server;
    EventLoop* _loop;
};

int main(){
    EventLoop loop; // epoll
    InetAddress addr("127.0.0.1", 6000);
    ChatServer server(&loop, addr, "Chatserver");

    server.start(); // listenfd  epoll_ctl=>epoll
    loop.loop();// epoll_wait 阻塞等待链接

    return 0;
}