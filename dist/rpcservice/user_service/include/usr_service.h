
#pragma once

#include "servicebase.h"
#include "usr.pb.h"
#include "usermodel.hpp"
#include "connectionpool.h"
#include <muduo/base/Logging.h>

// 用户服务实现
class UserServiceImpl : public ServiceBase, public userservice::UserService
{
public:
    UserServiceImpl(const std::string& ip, uint16_t port);
    
protected:
    // 实现服务基类的虚函数
    void InitRpcService() override;
    void InitDatabasePool() override;
    
    // 实现UserService的RPC方法
    void Login(::google::protobuf::RpcController* controller,
               const ::userservice::LoginRequest* request,
               ::userservice::LoginResponse* response,
               ::google::protobuf::Closure* done) override;
               
    void Register(::google::protobuf::RpcController* controller,
                  const ::userservice::RegisterRequest* request,
                  ::userservice::RegisterResponse* response,
                  ::google::protobuf::Closure* done) override;
                  
    void Logout(::google::protobuf::RpcController* controller,
                const ::userservice::LogoutRequest* request,
                ::userservice::LogoutResponse* response,
                ::google::protobuf::Closure* done) override;
                
    void GetUserInfo(::google::protobuf::RpcController* controller,
                     const ::userservice::GetUserInfoRequest* request,
                     ::userservice::GetUserInfoResponse* response,
                     ::google::protobuf::Closure* done) override;
                     
    void UpdateUserState(::google::protobuf::RpcController* controller,
                         const ::userservice::UpdateUserStateRequest* request,
                         ::userservice::UpdateUserStateResponse* response,
                         ::google::protobuf::Closure* done) override;

private:
    // 数据模型
    UserModel _userModel;
    
    // 数据库连接池
    ConnectionPool* _connectionPool;
};