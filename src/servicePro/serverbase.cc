#include "servicebase.h"
#include <iostream>
#include <muduo/base/Logging.h>

ServiceBase::ServiceBase(const std::string& serviceName, 
                         const std::string& ip, 
                         uint16_t port)
    : _serviceName(serviceName)
    , _ip(ip)
    , _port(port)
    , _server(&_loop, muduo::net::InetAddress(ip, port), serviceName)
{
    // 设置线程数
    _server.setThreadNum(4);
    
    // 初始化监控和熔断器
    InitMonitor();
    InitCircuitBreaker();
    
    LOG_INFO << "ServiceBase created for " << serviceName 
             << " on " << ip << ":" << port;
}

void ServiceBase::Start()
{
    std::cout << "Starting service: " << _serviceName 
              << " on " << _ip << ":" << _port << std::endl;
    
    // 初始化RPC服务
    InitRpcService();
    
    // 初始化数据库连接池
    InitDatabasePool();
    
    // 初始化Redis连接
    InitRedis();
    
    // 启动服务器
    _server.start();
    
    // 启动事件循环
    _loop.loop();
}

void ServiceBase::Stop()
{
    _loop.quit();
}

void ServiceBase::InitMonitor()
{
    _monitor = std::make_unique<ServiceMonitor>(_serviceName);
    LOG_INFO << "Monitor initialized for " << _serviceName;
}

void ServiceBase::InitCircuitBreaker()
{
    _circuitBreaker = std::make_unique<CircuitBreaker>();
    LOG_INFO << "Circuit breaker initialized for " << _serviceName;
}