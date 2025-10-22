#include "include/usr_service.h"
#include "mprpcprovider.h"
#include "logger.h"

UserServiceImpl::UserServiceImpl(const std::string& ip, uint16_t port)
    : ServiceBase("UserService", ip, port)
    , _connectionPool(nullptr)
{
    LOG_INFO("UserServiceImpl created");
}

void UserServiceImpl::InitRpcService()
{
    // 不作为RPC服务提供者运行
    LOG_INFO("UserService RPC service initialized");
}

void UserServiceImpl::InitDatabasePool()
{
    // 初始化数据库连接池
    _connectionPool = ConnectionPool::getConnectionPool();
    LOG_INFO("UserService database pool initialized");
}

void UserServiceImpl::Login(::google::protobuf::RpcController* controller,
                           const ::userservice::LoginRequest* request,
                           ::userservice::LoginResponse* response,
                           ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::Login called, id: %d", request->id());
    
    int id = request->id();
    std::string password = request->password();
    
    // 查询用户信息
    User user = _userModel.dbquery(id);
    
    if (user.getId() == id && user.getPwd() == password)
    {
        if (user.getState() == "online")
        {
            // 用户已经在线
            response->set_error_code(2);
            response->set_error_msg("this account is using, input another!");
        }
        else
        {
            // 登录成功
            response->set_error_code(0);
            response->set_id(user.getId());
            response->set_name(user.getName());
            response->set_state(user.getState());
            
            // 更新用户状态为在线
            user.setState("online");
            _userModel.updateState(user);
        }
    }
    else
    {
        // 用户不存在或密码错误
        response->set_error_code(1);
        response->set_error_msg("id or password is invalid!");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::Register(::google::protobuf::RpcController* controller,
                              const ::userservice::RegisterRequest* request,
                              ::userservice::RegisterResponse* response,
                              ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::Register called, name: %s", request->name().c_str());
    
    std::string name = request->name();
    std::string password = request->password();
    
    User user;
    user.setName(name);
    user.setPwd(password);
    
    if (_userModel.insert(user))
    {
        // 注册成功
        response->set_error_code(0);
        response->set_id(user.getId());
    }
    else
    {
        // 注册失败
        response->set_error_code(1);
        response->set_error_msg("register failed");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::Logout(::google::protobuf::RpcController* controller,
                            const ::userservice::LogoutRequest* request,
                            ::userservice::LogoutResponse* response,
                            ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::Logout called, id: %d", request->id());
    
    int id = request->id();
    
    // 更新用户状态为离线
    User user(id, "", "", "offline");
    if (_userModel.updateState(user))
    {
        response->set_error_code(0);
    }
    else
    {
        response->set_error_code(1);
        response->set_error_msg("update user state failed");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::GetUserInfo(::google::protobuf::RpcController* controller,
                                 const ::userservice::GetUserInfoRequest* request,
                                 ::userservice::GetUserInfoResponse* response,
                                 ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::GetUserInfo called, id: %d", request->id());
    
    int id = request->id();
    
    // 查询用户信息
    User user = _userModel.dbquery(id);
    
    if (user.getId() == id)
    {
        response->set_error_code(0);
        response->set_id(user.getId());
        response->set_name(user.getName());
        response->set_state(user.getState());
    }
    else
    {
        response->set_error_code(1);
        response->set_error_msg("user not found");
    }
    
    // 调用回调函数
    done->Run();
}

void UserServiceImpl::UpdateUserState(::google::protobuf::RpcController* controller,
                                     const ::userservice::UpdateUserStateRequest* request,
                                     ::userservice::UpdateUserStateResponse* response,
                                     ::google::protobuf::Closure* done)
{
    LOG_INFO("UserService::UpdateUserState called, id: %d, state: %s", request->id(), request->state().c_str());
    
    int id = request->id();
    std::string state = request->state();
    
    User user(id, "", "", state);
    if (_userModel.updateState(user))
    {
        response->set_error_code(0);
    }
    else
    {
        response->set_error_code(1);
        response->set_error_msg("update user state failed");
    }
    
    // 调用回调函数
    done->Run();
}