#pragma once
#include "google/protobuf/service.h"
#include <google/protobuf/descriptor.h>
#include <unordered_map>

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <muduo/net/TcpConnection.h>
#include <functional>

#include<zookeeperutil.h>

#include"mprpcheader.pb.h"

// 框架提供的专门发布rpc服务的网络对象类
class MprpcProvider
{
public:
    // 发布服务接口
    void NotifyService(google::protobuf::Service *service);
    // 开启节点 提供RPC服务
    void StartMprpc();
   
private:
    muduo::net::EventLoop _event_loop;

    // 服务类型信息(方法信息)
    struct MethodStruct
    {
        google::protobuf::Service *_service;                                                  // 服务对象(服务名称)
        std::unordered_map<std::string, const google::protobuf::MethodDescriptor*> _methodInfoMap; // 服务方法
    };
    // 服务对象及其方法信息 <service methodstruct>
    std::unordered_map<std::string, MethodStruct> _serviceInfo;

    // 连接回调
    void OnConnection(const muduo::net::TcpConnectionPtr &);
    // 读写回调
    void OnMessage(const muduo::net::TcpConnectionPtr &, muduo::net::Buffer *, muduo::Timestamp);
     // Closure的回调操作，用于序列化rpc的响应和网络发送
    void SendmprpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response);
    // 解析header头
    bool Paresrpcheader(const std::string& headerstr, std::string& serviceName, std::string& methodName, uint32_t &);
};
