#pragma once

#include <google/protobuf/service.h>
#include<google/protobuf/descriptor.h>

/*RpcChannel* channel = new MyRpcChannel("remotehost.example.com:1234");
MyService* service = new MyService::Stub(channel);
service->MyMethod(request, &response, callback);*/
class Mprpcchannel : public google::protobuf::RpcChannel
{
public:
    // 所有通过stub代理对象调用的rpc方法，都走到这里了，统一做rpc方法调用的数据数据序列化和网络发送 
    void CallMethod(const google::protobuf::MethodDescriptor* method,
                          google::protobuf::RpcController* controller, 
                          const google::protobuf::Message* request,
                          google::protobuf::Message* response, 
                          google::protobuf::Closure* done);
private:
    // 设置超时
    void Setimeout(int sockfd, int timeoutSec);
    // 设置服务端地址
    bool GetSeverAddr(const std::string& serviceName, const std::string& methodName, std::string& ip, uint16_t& port);
};
