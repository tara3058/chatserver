// dist/services/user_service/user_service.cpp
#include "include/usr_service.h"
#include "mprpcprovider.h"
#include"logger.h"
//#include <muduo/base/Logging.h>

UserServiceImpl::UserServiceImpl(const std::string& ip, uint16_t port)
    : ServiceBase("UserService", ip, port)
    , _connectionPool(nullptr)
{
    LOG_INFO ("UserServiceImpl created");
}

void UserServiceImpl::InitRpcService()
{
    // 创建RPC服务提供者并注册服务
    MprpcProvider provider;
    provider.NotifyService(this);
    LOG_INFO ( "UserService RPC service initialized");
}

void UserServiceImpl::InitDatabasePool()
{
    // 初始化数据库连接池
    _connectionPool = ConnectionPool::getConnectionPool();
    LOG_INFO ("UserService database pool initialized");
}

void UserServiceImpl::Login(::google::protobuf::RpcController* controller,
                           const ::userservice::LoginRequest* request,
                           ::userservice::LoginResponse* response,
                           ::google::protobuf::Closure* done)
{
    LOG_INFO ( "UserService::Login called, id:");
    
    int id = request->id();
    std::string password = request->password();
    
    // 查询用户信息
    User user = _userModel.dbquery(id);
    
    if (user.getId() == id && user.getPwd() == password)
    {
        if (user.getState() == "online")
        {
            // 用户已经在线
            userservice::ResultCode *resACK = response->mutable_result();
            resACK->set_errcode(2);
            resACK->set_errmsg("this account is using, input another!");
        }
        else
        {
            // 登录成功
            userservice::ResultCode *resACK = response->mutable_result();
            resACK->set_errcode(0);
            resACK->set_errmsg("");
            response->set_id(user.getId());
            response->set_name(user.getName());
            //response->set_state(user.getState());
            
            // 更新用户状态为在线
            user.setState("online");
            _userModel.updateState(user);
        }
    }
    else
    {
        // 用户不存在或密码错误
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(2);
        resACK->set_errmsg("id or password is invalid!");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::Register(::google::protobuf::RpcController* controller,
                              const ::userservice::RegisterRequest* request,
                              ::userservice::RegisterResponse* response,
                              ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::Register called, name");
    
    std::string name = request->name();
    std::string password = request->password();
    
    User user;
    user.setName(name);
    user.setPwd(password);
    
    if (_userModel.insert(user))
    {
        // 注册成功
       userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(0);
        resACK->set_errmsg("");
        response->set_id(user.getId());
    }
    else
    {
        // 注册失败
       userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(1);
        resACK->set_errmsg("register failed");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::Logout(::google::protobuf::RpcController* controller,
                            const ::userservice::LogoutRequest* request,
                            ::userservice::LogoutResponse* response,
                            ::google::protobuf::Closure* done)
{
    LOG_INFO ( "UserService::Logout called, id: " );
    
    int id = request->id();
    
    // 更新用户状态为离线
    User user(id, "", "", "offline");
    if (_userModel.updateState(user))
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(0);
        resACK->set_errmsg("");
    }
    else
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(1);
        resACK->set_errmsg("update user state failed");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::GetUserInfo(::google::protobuf::RpcController* controller,
                                 const ::userservice::GetUserInfoRequest* request,
                                 ::userservice::GetUserInfoResponse* response,
                                 ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::GetUserInfo called, id:");
    
    int id = request->id();
    
    // 查询用户信息
    User user = _userModel.dbquery(id);
    
    if (user.getId() == id)
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(0);
        response->set_id(user.getId());
        response->set_name(user.getName());
        response->set_state(user.getState());
    }
    else
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(1);
        resACK->set_errmsg("user not found");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::UpdateUserInfo(::google::protobuf::RpcController* controller,
                                     const ::userservice::UpdateUserInfoRequest* request,
                                     ::userservice::UpdateUserInfoResponse* response,
                                     ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::UpdateUserState called" );
    
    int id = request->id();
    std::string state = request->state();
    
    User user(id, "", "", state);
    if (_userModel.updateState(user))
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(0);
        resACK->set_errmsg("");
    }
    else
    {
        userservice::ResultCode *resACK = response->mutable_result();
        resACK->set_errcode(1);
        resACK->set_errmsg("update user state failed");
    }
    
    // 调用回调函数
    done->Run();
}