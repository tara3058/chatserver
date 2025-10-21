// dist/rpcservice/gateway/src/gateway.cc
#include "gateway.h"
#include <muduo/base/Logging.h>
#include "public.hpp"

GatewayService::GatewayService(const std::string& ip, uint16_t port)
    : ServiceBase("GatewayService", ip, port)
{
    // 初始化消息处理器
    InitMsgHandlers();
    
    LOG_INFO << "GatewayService created";
}

void GatewayService::InitRpcService()
{
    // 初始化RPC存根
    _userStub = std::make_unique<userservice::UserService::Stub>(&_userRpcChannel);
    //_messageStub = std::make_unique<messageservice::MessageService::Stub>(&_messageRpcChannel);
    //_relationStub = std::make_unique<relationservice::RelationService::Stub>(&_relationRpcChannel);
    
    // 设置服务器连接和消息回调
    _server.setConnectionCallback(
        std::bind(&GatewayService::OnConnection, this, std::placeholders::_1));
    _server.setMessageCallback(
        std::bind(&GatewayService::OnMessage, this, std::placeholders::_1, 
                  std::placeholders::_2, std::placeholders::_3));
    
    LOG_INFO << "GatewayService RPC service initialized";
}

void GatewayService::InitMsgHandlers()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&GatewayService::HandleLogin, this, 
                                               std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&GatewayService::HandleRegister, this, 
                                             std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&GatewayService::HandleOneChat, this, 
                                                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_FRIEND_ACK, std::bind(&GatewayService::HandleAddFriend, this, 
                                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({CREATE_GROUP_ACK, std::bind(&GatewayService::HandleCreateGroup, this, 
                                                      std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({ADD_GROUP_ACK, std::bind(&GatewayService::HandleAddGroup, this, 
                                                   std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&GatewayService::HandleGroupChat, this, 
                                                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&GatewayService::HandleLoginOut, this, 
                                                  std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)});
    
    LOG_INFO << "Message handlers initialized";
}

void GatewayService::OnConnection(const muduo::net::TcpConnectionPtr& conn)
{
    if (!conn->connected())
    {
        LOG_INFO << "Client disconnected: " << conn->peerAddress().toIpPort();
        
        // 处理客户端异常断开
        int userId = -1;
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            for (auto it = _userConnMap.begin(); it != _userConnMap.end(); ++it)
            {
                if (it->second == conn)
                {
                    userId = it->first;
                    _userConnMap.erase(it);
                    break;
                }
            }
        }
        
        if (userId != -1)
        {
            // 调用用户服务更新用户状态
            userservice::UpdateUserInfoRequest request;
            request.set_id(userId);
            request.set_state("offline");
            
            userservice::UpdateUserInfoResponse response;
            MprpcController controller;
            
            _userStub->UpdateUserState(nullptr, &request, &response, nullptr);
        }
        
        conn->shutdown();
    }
    else
    {
        LOG_INFO << "New client connected: " << conn->peerAddress().toIpPort();
    }
}

void GatewayService::OnMessage(const muduo::net::TcpConnectionPtr& conn,
                              muduo::net::Buffer* buffer,
                              muduo::Timestamp time)
{
    std::string buf = buffer->retrieveAllAsString();
    LOG_DEBUG << "Received message: " << buf;
    
    try {
        json js = json::parse(buf);
        int msgid = js["msgid"].get<int>();
        
        auto msgHandler = GetHandler(msgid);
        msgHandler(conn, js, time);
    }
    catch (const std::exception& e) {
        LOG_ERROR << "Failed to parse message: " << e.what();
        json response;
        response["msgid"] = ERROR_MSG;
        response["errmsg"] = "Invalid message format";
        conn->send(response.dump());
    }
}

MsgHandler GatewayService::GetHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp)
        {
            LOG_ERROR << "Unknown message id: " << msgid;
            json response;
            response["msgid"] = ERROR_MSG;
            response["errmsg"] = "Unknown message type";
            conn->send(response.dump());
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

void GatewayService::HandleLogin(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    int id = js["id"].get<int>();
    std::string pwd = js["password"];
    
    LOG_INFO << "User login request, id: " << id;
    
    // 调用用户服务进行登录验证
    userservice::LoginRequest request;
    request.set_id(id);
    request.set_password(pwd);
    
    userservice::LoginResponse response;
    MprpcController controller;
    
    _userStub->Login(nullptr, &request, &response, nullptr);
    
    if (response.error_code() == 0)
    {
        // 登录成功
        {
            std::lock_guard<std::mutex> lock(_connMutex);
            _userConnMap.insert({id, conn});
        }
        
        json responseJson;
        responseJson["msgid"] = LOGIN_MSG_ACK;
        responseJson["errno"] = 0;
        responseJson["id"] = response.id();
        responseJson["name"] = response.name();
        responseJson["state"] = response.state();
        
        conn->send(responseJson.dump());
        LOG_INFO << "User " << id << " login successful";
    }
    else
    {
        // 登录失败
        json responseJson;
        responseJson["msgid"] = LOGIN_MSG_ACK;
        responseJson["errno"] = response.error_code();
        responseJson["errmsg"] = response.error_msg();
        
        conn->send(responseJson.dump());
        LOG_INFO << "User " << id << " login failed: " << response.error_msg();
    }
}

void GatewayService::HandleRegister(const muduo::net::TcpConnectionPtr& conn, json& js, muduo::Timestamp time)
{
    std::string name = js["name"];
    std::string pwd = js["password"];
    
    LOG_INFO << "User register request, name: " << name;
    
    // 调用用户服务进行注册
    userservice::RegisterRequest request;
    request.set_name(name);
    request.set_password(pwd);
    
    userservice::RegisterResponse response;
    MprpcController controller;
    
    _userStub->Register(nullptr, &request, &response, nullptr);
    
    json responseJson;
    responseJson["msgid"] = REG_MSG_ACK;
    responseJson["errno"] = response.error_code();
    
    if (response.error_code() == 0)
    {
        responseJson["id"] = response.id();
        LOG_INFO << "User " << name << " registered successfully with id: " << response.id();
    }
    else
    {
        responseJson["errmsg"] = response.error_msg();
        LOG_INFO << "User " << name << " registration failed: " << response.error_msg();
    }
    
    conn->send(responseJson.dump());
}

