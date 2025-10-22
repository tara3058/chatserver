// dist/rpcservice/message/message.h
#pragma once

#include "servicebase.h"
#include "messege.pb.h"
#include "offlinemsgmodel.hpp"
#include "connectionpool.h"
#include <muduo/base/Logging.h>

// 消息服务实现
class MessageServiceImpl : public ServiceBase, public messageservice::MessageService
{
public:
    MessageServiceImpl(const std::string& ip, uint16_t port);
    
protected:
    // 实现服务基类的虚函数
    void InitRpcService() override;
    void InitDatabasePool() override;
    
    // 实现MessageService的RPC方法
    void SendOneToOneMessage(::google::protobuf::RpcController* controller,
                            const ::messageservice::OneToOneMessageRequest* request,
                            ::messageservice::OneToOneMessageResponse* response,
                            ::google::protobuf::Closure* done) override;
                            
    void SendGroupMessage(::google::protobuf::RpcController* controller,
                         const ::messageservice::GroupMessageRequest* request,
                         ::messageservice::GroupMessageResponse* response,
                         ::google::protobuf::Closure* done) override;
                         
    void StoreOfflineMessage(::google::protobuf::RpcController* controller,
                            const ::messageservice::StoreOfflineMessageRequest* request,
                            ::messageservice::StoreOfflineMessageResponse* response,
                            ::google::protobuf::Closure* done) override;
                            
    void GetOfflineMessages(::google::protobuf::RpcController* controller,
                           const ::messageservice::GetOfflineMessagesRequest* request,
                           ::messageservice::GetOfflineMessagesResponse* response,
                           ::google::protobuf::Closure* done) override;
                           
    void RemoveOfflineMessages(::google::protobuf::RpcController* controller,
                              const ::messageservice::RemoveOfflineMessagesRequest* request,
                              ::messageservice::RemoveOfflineMessagesResponse* response,
                              ::google::protobuf::Closure* done) override;

private:
    // 离线消息模型
    OfflineMsgModel _offlineMsgModel;
    
    // 数据库连接池
    ConnectionPool* _connectionPool;
};