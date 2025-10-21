#include "mprpcapplication.h"
#include "user.pb.h"
#include <iostream>

int main(int argc, char **argv)
{
    // 框架初始化
    MprpcApplication::Init(argc, argv);

    // rpc方法的请求参数
    fixbug::LoginRequest request;
    request.set_name("zhang san");
    request.set_pwd("123456");
    // rpc方法的响应
    fixbug::LoginResponse response;

    // 调用rpc方法
    MprpcController controller;
    fixbug::UserServiceRPC_Stub serviceStub(new Mprpcchannel());
    serviceStub.Login(&controller, &request, &response, nullptr);

    // 读调用的结果
    if (controller.Failed())
    {
        //调用执行失败
        std::cout << controller.ErrorText() << std::endl;
    }
    else
    {
        //执行成功 查看response
        if (0 == response.response().errcode())
        {
            std::cout << "rpc login response success:" << response.success() << std::endl;
        }
        else
        {
            std::cout << "rpc login response error : " << response.response().errmsg() << std::endl;
        }
    }

    return 0;
}