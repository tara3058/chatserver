#pragma once

#include <muduo/net/EventLoop.h>
#include <muduo/net/InetAddress.h>
#include <muduo/net/TcpServer.h>
#include <string>
#include <memory>
#include "monitor.h"
#include "circuitbreaker.h"

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

    // 获取服务监控器
    ServiceMonitor& GetMonitor() { return *_monitor; }
    
    // 获取熔断器
    CircuitBreaker& GetCircuitBreaker() { return *_circuitBreaker; }
    
    // 获取TCP服务器引用（供子类设置回调）
    muduo::net::TcpServer& GetServer() { return _server; }

protected:
    // 初始化RPC服务
    virtual void InitRpcService() = 0;
    
    // 初始化数据库连接池
    virtual void InitDatabasePool() {}
    
    // 初始化Redis连接
    virtual void InitRedis() {}

    // 初始化监控
    virtual void InitMonitor();
    
    // 初始化熔断器
    virtual void InitCircuitBreaker();

private:
    std::string _serviceName;      // 服务名称
    std::string _ip;               // 监听IP
    uint16_t _port;                // 监听端口
    muduo::net::EventLoop _loop;   // 事件循环
    muduo::net::TcpServer _server; // TCP服务器

    // 监控器
    std::unique_ptr<ServiceMonitor> _monitor;
    
    // 熔断器
    std::unique_ptr<CircuitBreaker> _circuitBreaker;
};