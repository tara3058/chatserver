#include "message.h"
#include "mprpcprovider.h"
#include "logger.h"
#include <muduo/base/Logging.h>
#include <iostream>

MessageServiceImpl::MessageServiceImpl(const std::string& ip, uint16_t port)
    : ServiceBase("MessageService", ip, port), _connectionPool(nullptr)
{
    std::cout << "MessageServiceImpl created" << std::endl;
}

// 实现服务基类的虚函数
void MessageServiceImpl::InitRpcService(){
    MprpcProvider provider;
    provider.NotifyService(this);
    std::cout << "MessageService RPC service initialized" << std::endl;
}

void MessageServiceImpl::InitDatabasePool(){
    // 初始化数据库连接池
    _connectionPool = ConnectionPool::getConnectionPool();
    std::cout << "MessageService database pool initialized" << std::endl;
}

// 实现MessageService的RPC方法
void MessageServiceImpl::SendOneToOneMessage(::google::protobuf::RpcController* controller,
                        const ::messageservice::OneToOneMessageRequest* request,
                        ::messageservice::OneToOneMessageResponse* response,
                        ::google::protobuf::Closure* done)
{
    std::cout << "MessageService::SendOneToOneMessage called" << std::endl;
    // 获取数据库连接
    auto conn = _connectionPool->getConnection();
    
    // TODO: 实现发送一对一消息的逻辑
    // 这里应该包含实际的消息发送和存储逻辑
    
    // 设置响应
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void MessageServiceImpl::SendGroupMessage(::google::protobuf::RpcController* controller,
                        const ::messageservice::GroupMessageRequest* request,
                        ::messageservice::GroupMessageResponse* response,
                        ::google::protobuf::Closure* done)
{
    std::cout << "MessageService::SendGroupMessage called" << std::endl;
    // 获取数据库连接
    auto conn = _connectionPool->getConnection();
    
    // TODO: 实现发送群组消息的逻辑
    
    // 设置响应
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void MessageServiceImpl::StoreOfflineMessage(::google::protobuf::RpcController* controller,
                        const ::messageservice::StoreOfflineMessageRequest* request,
                        ::messageservice::StoreOfflineMessageResponse* response,
                        ::google::protobuf::Closure* done)
{
    std::cout << "MessageService::StoreOfflineMessage called" << std::endl;
    
    // 存储离线消息
    bool result = _offlineMsgModel.insert(request->user_id(), request->message());
    
    if (result) {
        response->set_error_code(0);
        response->set_error_msg("Success");
    } else {
        response->set_error_code(1);
        response->set_error_msg("Failed to store offline message");
    }
    
    // 完成RPC调用
    done->Run();
}
                        
void MessageServiceImpl::GetOfflineMessages(::google::protobuf::RpcController* controller,
                        const ::messageservice::GetOfflineMessagesRequest* request,
                        ::messageservice::GetOfflineMessagesResponse* response,
                        ::google::protobuf::Closure* done)
{
    std::cout << "MessageService::GetOfflineMessages called" << std::endl;
    
    // 获取离线消息
    std::vector<std::string> messages = _offlineMsgModel.query(request->user_id());
    
    // 添加到响应中
    for (const auto& msg : messages) {
        response->add_messages(msg);
    }
    
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void MessageServiceImpl::RemoveOfflineMessages(::google::protobuf::RpcController* controller,
                            const ::messageservice::RemoveOfflineMessagesRequest* request,
                            ::messageservice::RemoveOfflineMessagesResponse* response,
                            ::google::protobuf::Closure* done)
{
    std::cout << "MessageService::RemoveOfflineMessages called" << std::endl;
    
    // 删除离线消息
    bool result = _offlineMsgModel.remove(request->user_id());
    
    if (result) {
        response->set_error_code(0);
        response->set_error_msg("Success");
    } else {
        response->set_error_code(1);
        response->set_error_msg("Failed to remove offline messages");
    }
    
    // 完成RPC调用
    done->Run();
}