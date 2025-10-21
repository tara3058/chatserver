// dist/rpcservice/gateway/include/gateway.h
#pragma once

#include "servicebase.h"
#include "json.hpp"
#include <muduo/net/TcpConnection.h>
#include <muduo/net/Buffer.h>
#include <unordered_map>
#include <functional>
#include <mutex>
#include "mprpcconsumer.h"
#include "../user_service/include/usr_service.h"
//#include "message_service.pb.h"
//#include "relation_service.pb.h"

using json = nlohmann::json;
using MsgHandler = std::function<void(const muduo::net::TcpConnectionPtr&, json&, muduo::Timestamp)>;

// 网关服务实现
class GatewayService : public ServiceBase
{
public:
    GatewayService(const std::string& ip, uint16_t port);
    
protected:
    // 实现服务基类的虚函数
    void InitRpcService() override;
    
    // TCP连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr& conn);
    
    // 消息处理回调
    void OnMessage(const muduo::net::TcpConnectionPtr& conn,
                   muduo::net::Buffer* buffer,
                   muduo::Timestamp time);
    
    // 获取消息处理器
    MsgHandler GetHandler(int msgid);

private:
    // 存储消息ID和对应的业务处理方法
    std::unordered_map<int, MsgHandler> _msgHandlerMap;
    
    // 存储在线用户的通信连接
    std::unordered_map<int, muduo::net::TcpConnectionPtr> _userConnMap;
    
    // 互斥锁，保证_userConnMap的线程安全
    std::mutex _connMutex;
    
    // RPC通道
    Mprpcchannel _userRpcChannel;
    Mprpcchannel _messageRpcChannel;
    Mprpcchannel _relationRpcChannel;
    
    // RPC存根
    std::unique_ptr<userservice::UserService::Stub> _userStub;
    //std::unique_ptr<messageservice::MessageService::Stub> _messageStub;
    //std::unique_ptr<relationservice::RelationService::Stub> _relationStub;
    
    // 初始化消息处理器
    void InitMsgHandlers();
    
    // 消息处理方法
    void HandleLogin(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleRegister(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleOneChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleAddFriend(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleCreateGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleAddGroup(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleGroupChat(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
    void HandleLoginOut(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time);
};