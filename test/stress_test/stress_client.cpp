#include <muduo/net/TcpClient.h>
#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/base/Logging.h>
#include <muduo/base/Thread.h>
#include <json.hpp>
#include <iostream>
#include <atomic>
#include <chrono>
#include <vector>
#include <random>
#include <memory>

using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

class StressTestClient {
public:
    StressTestClient(EventLoop* loop, const InetAddress& serverAddr, 
                     int clientId, int totalClients)
        : loop_(loop),
          client_(loop, serverAddr, "StressClient_" + std::to_string(clientId)),
          clientId_(clientId),
          totalClients_(totalClients),
          connected_(false),
          messageCount_(0),
          startTime_(std::chrono::steady_clock::now()) {
        
        client_.setConnectionCallback(
            std::bind(&StressTestClient::onConnection, this, std::placeholders::_1));
        client_.setMessageCallback(
            std::bind(&StressTestClient::onMessage, this, std::placeholders::_1,
                     std::placeholders::_2, std::placeholders::_3));
        
        // 随机用户ID
        userId_ = 1000 + (rand()%(8000-1000));
    }

    void start() {
        client_.connect();
    }

    void stop() {
        client_.disconnect();
    }

    int getMessageCount() const { return messageCount_; }
    bool isConnected() const { return connected_; }

private:
    void onConnection(const TcpConnectionPtr& conn) {
        if (conn->connected()) {
            LOG_INFO << "Client " << clientId_ << " connected to " << conn->peerAddress().toIpPort();
            connected_ = true;
            
            // 发送登录消息
            sendLoginMessage(conn);
            
            // 开始定时发送聊天消息（降低频率避免服务器过载）
            loop_->runEvery(0.5, std::bind(&StressTestClient::sendChatMessage, this, conn));
        } else {
            LOG_INFO << "Client " << clientId_ << " disconnected";
            connected_ = false;
        }
    }

    void onMessage(const TcpConnectionPtr& conn, Buffer* buf, Timestamp time) {
        std::string message = buf->retrieveAllAsString();
        try {
            json response = json::parse(message);
            messageCount_++;
            
            // 可以在这里处理服务器响应
            if (response.contains("msgid")) {
                int msgId = response["msgid"];
                // LOG_INFO << "Client " << clientId_ << " received response for msgId: " << msgId;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to parse response: " << e.what();
        }
    }

    void sendLoginMessage(const TcpConnectionPtr& conn) {
        json loginMsg;
        loginMsg["msgid"] = 1; // LOGIN_MSG
        loginMsg["id"] = userId_;
        loginMsg["password"] = "20";
        
        std::string msg = loginMsg.dump();
        // 添加换行符作为消息分隔符
        msg += "\n";
        conn->send(msg);
        LOG_INFO << "Client " << clientId_ << " sent login message: " << loginMsg.dump();
    }

    void sendChatMessage(const TcpConnectionPtr& conn) {
        if (!connected_) return;
        
        static std::random_device rd;
        static std::mt19937 gen(rd());
        static std::uniform_int_distribution<> dis(1001, 1000 + totalClients_);
        
        json chatMsg;
        chatMsg["msgid"] = 5; // ONE_CHAT_MSG
        chatMsg["id"] = userId_;
        chatMsg["from"] = userId_;
        chatMsg["to"] = dis(gen); // 随机选择接收者
        chatMsg["msg"] = "Hello from client " + std::to_string(clientId_) + 
                        " - message " + std::to_string(messageCount_);
        chatMsg["time"] = std::to_string(std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now().time_since_epoch()).count());
        
        std::string msg = chatMsg.dump();
        // 添加换行符作为消息分隔符
        msg += "\n";
        conn->send(msg);
    }

    EventLoop* loop_;
    TcpClient client_;
    int clientId_;
    int totalClients_;
    int userId_;
    bool connected_;
    std::atomic<int> messageCount_;
    std::chrono::steady_clock::time_point startTime_;
};

class StressTestController {
public:
    StressTestController(const std::string& serverIp, int serverPort, 
                        int clientCount, int testDuration)
        : serverAddr_(serverIp, serverPort),
          clientCount_(clientCount),
          testDuration_(testDuration),
          totalMessages_(0) {
    }

    void run() {
        LOG_INFO << "Starting stress test with " << clientCount_ << " clients";
        LOG_INFO << "Server: " << serverAddr_.toIpPort();
        LOG_INFO << "Test duration: " << testDuration_ << " seconds";
        
        // 创建事件循环
        EventLoop loop;
        
        // 创建客户端
        std::vector<std::unique_ptr<StressTestClient>> clients;
        for (int i = 0; i < clientCount_; ++i) {
            auto client = std::make_unique<StressTestClient>(&loop, serverAddr_, i, clientCount_);
            clients.push_back(std::move(client));
        }
        
        // 启动所有客户端
        for (auto& client : clients) {
            client->start();
        }
        
        // 设置测试结束定时器
        auto startTime = std::chrono::steady_clock::now();
        loop.runAfter(testDuration_, [&]() {
            auto endTime = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::seconds>(endTime - startTime);
            
            // 统计结果
            int connectedClients = 0;
            int totalMessages = 0;
            
            for (auto& client : clients) {
                if (client->isConnected()) {
                    connectedClients++;
                }
                totalMessages += client->getMessageCount();
            }
            
            LOG_INFO << "=== Stress Test Results ===";
            LOG_INFO << "Test duration: " << duration.count() << " seconds";
            LOG_INFO << "Total clients: " << clientCount_;
            LOG_INFO << "Connected clients: " << connectedClients;
            LOG_INFO << "Total messages sent: " << totalMessages;
            LOG_INFO << "Messages per second: " << totalMessages / duration.count();
            LOG_INFO << "Average messages per client: " << totalMessages / clientCount_;
            
            // 停止所有客户端
            for (auto& client : clients) {
                client->stop();
            }
            
            loop.quit();
        });
        
        loop.loop();
    }

private:
    InetAddress serverAddr_;
    int clientCount_;
    int testDuration_;
    std::atomic<int> totalMessages_;
};

int main(int argc, char* argv[]) {
    if (argc != 5) {
        std::cerr << "Usage: " << argv[0] << " <server_ip> <server_port> <client_count> <test_duration>" << std::endl;
        std::cerr << "Example: " << argv[0] << " 127.0.0.1 6000 100 60" << std::endl;
        return 1;
    }
    
    std::string serverIp = argv[1];
    int serverPort = std::atoi(argv[2]);
    int clientCount = std::atoi(argv[3]);
    int testDuration = std::atoi(argv[4]);
    
    if (clientCount <= 0 || clientCount > 10000) {
        std::cerr << "Client count should be between 1 and 10000" << std::endl;
        return 1;
    }
    
    if (testDuration <= 0 || testDuration > 3600) {
        std::cerr << "Test duration should be between 1 and 3600 seconds" << std::endl;
        return 1;
    }
    
    try {
        StressTestController controller(serverIp, serverPort, clientCount, testDuration);
        controller.run();
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
