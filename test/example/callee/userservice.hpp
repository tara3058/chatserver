#pragma once

#include<iostream>
#include<string>

#include"user.pb.h"

class Userserive : public fixbug::UserServiceRPC
{
public:
    //本地服务
    bool Login(std::string name, std::string pwd);
    //重写基类UserServiceRpc的虚函数
    void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done);



};
