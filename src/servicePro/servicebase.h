#pragma once

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <string>

// 服务基类
class ServiceBase
{
public:
    ServiceBase(const std::string& serviceName, 
                const std::string& ip, 
                uint16_t port);
    
    virtual ~ServiceBase() = default;
    
    // 启动服务
    void Start();
    
    // 停止服务
    void Stop();
    
    // 获取服务名称
    const std::string& GetServiceName() const { return _serviceName; }
    
    // 获取IP地址
    const std::string& GetIp() const { return _ip; }
    
    // 获取端口号
    uint16_t GetPort() const { return _port; }

protected:
    // 初始化RPC服务
    virtual void InitRpcService() = 0;
    
    // 初始化数据库连接池
    virtual void InitDatabasePool() {}
    
    // 初始化Redis连接
    virtual void InitRedis() {}

private:
    std::string _serviceName;      // 服务名称
    std::string _ip;               // 监听IP
    uint16_t _port;                // 监听端口
    muduo::net::EventLoop _loop;   // 事件循环
    muduo::net::TcpServer _server; // TCP服务器
};