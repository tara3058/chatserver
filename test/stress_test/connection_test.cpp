#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <json.hpp>
#include <iostream>
#include <chrono>
#include <thread>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

class SimpleTestClient {
public:
    SimpleTestClient(EventLoop* loop, const InetAddress& serverAddr)
        : loop_(loop),
          client_(loop, serverAddr, "TestClient"),
          connected_(false) {
        
        client_.setConnectionCallback(
            std::bind(&SimpleTestClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&SimpleTestClient::onMessage, this, std::placeholders::_1,
                     std::placeholders::_2, std::placeholders::_3));
    }

    void start() {
        client_.connect();
    }

    void stop() {
        client_.disconnect();
    }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO << "Connected to " << conn->peerAddress().toIpPort();
            connected_ = true;
            
            // 发送登录消息
            json loginMsg;
            loginMsg["msgid"] = 1; // LOGIN_MSG
            loginMsg["id"] = 1001;
            loginMsg["password"] = "123456";
            
            std::string msg = loginMsg.dump();
            conn->send(msg);
            LOG_INFO << "Login message sent: " << msg;
            
            // 3秒后发送聊天消息
            loop_->runAfter(3.0, [this, conn]() {
                this->sendChatMessage(conn);
            });
            
            // 6秒后断开连接
            loop_->runAfter(6.0, [this]() {
                this->stop();
                loop_->quit();
            });
            
        } else {
            LOG_INFO << "Disconnected";
            connected_ = false;
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        std::string message = buf->retrieveAllAsString();
        LOG_INFO << "Received: " << message;
        
        try {
            json response = json::parse(message);
            if (response.contains("msgid") && response.contains("errno")) {
                int msgId = response["msgid"];
                int errNo = response["errno"];
                LOG_INFO << "Response - MsgId: " << msgId << ", Error: " << errNo;
                
                if (errNo == 0) {
                    LOG_INFO << "Operation successful";
                } else {
                    LOG_ERROR << "Operation failed with error: " << errNo;
                    if (response.contains("errmsg")) {
                        LOG_ERROR << "Error message: " << response["errmsg"].get<std::string>();
                    }
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse response: " << e.what();
        }
    }

    void sendChatMessage(const TcpConnectionPtr& conn) {
        json chatMsg;
        chatMsg["msgid"] = 5; // ONE_CHAT_MSG
        chatMsg["id"] = 1001;
        chatMsg["from"] = 1001;
        chatMsg["to"] = 1002;
        chatMsg["msg"] = "Hello, this is a test message!";
        chatMsg["time"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        std::string msg = chatMsg.dump();
        conn->send(msg);
        LOG_INFO << "Chat message sent: " << msg;
    }

    EventLoop* loop_;
    TcpClient client_;
    bool connected_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 6000" << std::endl;
        return 1;
    }
    
    std::string serverIp = argv[1];
    int serverPort = std::atoi(argv[2]);
    
    LOG_INFO << "Connecting to chat server at " << serverIp << ":" << serverPort;
    
    EventLoop loop;
    InetAddress serverAddr(serverIp, serverPort);
    SimpleTestClient client(&loop, serverAddr);
    
    client.start();
    loop.loop();
    
    LOG_INFO << "Test completed";
    return 0;
}