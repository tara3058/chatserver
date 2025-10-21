#include "userservice.hpp"


/* 原为本地登录服务*/

bool Userserive::Login(std::string name, std::string pwd)
{
    std::cout << "doing local service: Login" << std::endl;
    std::cout << "name:" << name << " pwd:" << pwd << std::endl;
    return true;
}

void Userserive::Login(::google::protobuf::RpcController *controller,
                       const ::fixbug::LoginRequest *request,
                       ::fixbug::LoginResponse *response,
                       ::google::protobuf::Closure *done)
{
    //获取框架给业务上报的请求参数，用于本地业务
    std::string name = request->name();
    std::string pwd = request->pwd();

    //做本地业务
    bool loginRse = Login(name, pwd);

    //写入业务结果
    fixbug::ResultCode *resACK = response->mutable_response();
    resACK->set_errcode(0);
    resACK->set_errmsg("");
    response->set_success(loginRse);

     // 执行回调操作   框架完成（响应对象数据的序列化和网络发送）
     done->Run();


}
