#pragma once

#include "servicebase.h"
#include "usr.pb.h"
#include "usermodel.hpp"
#include "connectionpool.h"
//#include <muduo/base/Logging.h>

// 用户服务实现
class UserServiceImpl : public ServiceBase, public userservice::UserService
{
public:
    UserServiceImpl(const std::string& ip, uint16_t port);
    //~UserServiceImpl() = default;
    
protected:
    // 实现服务基类的虚函数
    void InitRpcService() override;
    void InitDatabasePool() override;
    
    // 实现UserService的RPC方法
    // 用户登录
    void Login(::google::protobuf::RpcController* controller,
               const ::userservice::LoginRequest* request,
               ::userservice::LoginResponse* response,
               ::google::protobuf::Closure* done) override;
    // 用户注册     
    void Register(::google::protobuf::RpcController* controller,
                  const ::userservice::RegisterRequest* request,
                  ::userservice::RegisterResponse* response,
                  ::google::protobuf::Closure* done) override;
                  
    void Logout(::google::protobuf::RpcController* controller,
                const ::userservice::LogoutRequest* request,
                ::userservice::LogoutResponse* response,
                ::google::protobuf::Closure* done) override;
    // 获取用户信息           
    void GetUserInfo(::google::protobuf::RpcController* controller,
                     const ::userservice::GetUserInfoRequest* request,
                     ::userservice::GetUserInfoResponse* response,
                     ::google::protobuf::Closure* done) override;
    //更新用户信息                
    void UpdateUserInfo(::google::protobuf::RpcController* controller,
                         const ::userservice::UpdateUserInfoRequest* request,
                         ::userservice::UpdateUserInfoResponse* response,
                         ::google::protobuf::Closure* done) override;

private:
    // 数据模型
    UserModel _userModel;
    
    // 数据库连接池
    ConnectionPool* _connectionPool;
};