// src/servicePro/connectionpool.h
#pragma once

#include <queue>
#include <mutex>
#include <memory>
#include <chrono>
#include <muduo/base/Logging.h>

// 连接池中的连接信息
template<typename Connection>
struct PooledConnection {
    std::shared_ptr<Connection> connection;
    std::chrono::steady_clock::time_point lastUsed;
    bool inUse;
    
    PooledConnection(std::shared_ptr<Connection> conn) 
        : connection(conn), lastUsed(std::chrono::steady_clock::now()), inUse(true) {}
};

// 通用连接池模板
template<typename Connection>
class ConnectionPool
{
public:
    ConnectionPool(size_t maxSize = 10, 
                   std::chrono::seconds maxIdleTime = std::chrono::seconds(300));
    
    ~ConnectionPool();
    
    // 获取连接
    std::shared_ptr<Connection> GetConnection();
    
    // 归还连接
    void ReturnConnection(std::shared_ptr<Connection> conn);
    
    // 创建新连接（需要子类实现）
    virtual std::shared_ptr<Connection> CreateConnection() = 0;
    
    // 验证连接有效性（需要子类实现）
    virtual bool ValidateConnection(std::shared_ptr<Connection> conn) = 0;
    
    // 关闭连接（需要子类实现）
    virtual void CloseConnection(std::shared_ptr<Connection> conn) = 0;
    
private:
    // 清理过期连接
    void CleanupExpiredConnections();
    
    // 最大连接数
    const size_t maxSize_;
    
    // 最大空闲时间
    const std::chrono::seconds maxIdleTime_;
    
    // 连接队列
    std::queue<std::shared_ptr<PooledConnection<Connection>>> availableConnections_;
    
    // 正在使用的连接数
    std::atomic<size_t> inUseCount_;
    
    // 互斥锁
    std::mutex mutex_;
};

template<typename Connection>
ConnectionPool<Connection>::ConnectionPool(size_t maxSize, std::chrono::seconds maxIdleTime)
    : maxSize_(maxSize), maxIdleTime_(maxIdleTime), inUseCount_(0)
{
    LOG_INFO << "ConnectionPool created with maxSize=" << maxSize 
             << ", maxIdleTime=" << maxIdleTime.count() << "s";
}

template<typename Connection>
ConnectionPool<Connection>::~ConnectionPool()
{
    std::lock_guard<std::mutex> lock(mutex_);
    while (!availableConnections_.empty()) {
        auto pooledConn = availableConnections_.front();
        CloseConnection(pooledConn->connection);
        availableConnections_.pop();
    }
    LOG_INFO << "ConnectionPool destroyed";
}

template<typename Connection>
std::shared_ptr<Connection> ConnectionPool<Connection>::GetConnection()
{
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 清理过期连接
    CleanupExpiredConnections();
    
    // 如果有可用连接，直接返回
    while (!availableConnections_.empty()) {
        auto pooledConn = availableConnections_.front();
        availableConnections_.pop();
        
        // 验证连接有效性
        if (ValidateConnection(pooledConn->connection)) {
            pooledConn->inUse = true;
            pooledConn->lastUsed = std::chrono::steady_clock::now();
            inUseCount_++;
            LOG_DEBUG << "Reusing connection from pool";
            return pooledConn->connection;
        } else {
            // 连接无效，关闭并移除
            CloseConnection(pooledConn->connection);
            LOG_DEBUG << "Removed invalid connection from pool";
        }
    }
    
    // 如果没有可用连接且未达到最大连接数，创建新连接
    if (inUseCount_.load() < maxSize_) {
        try {
            auto newConn = CreateConnection();
            if (newConn) {
                inUseCount_++;
                LOG_DEBUG << "Created new connection";
                return newConn;
            }
        } catch (const std::exception& e) {
            LOG_ERROR << "Failed to create connection: " << e.what();
        }
    }
    
    LOG_WARN << "No available connections in pool";
    return nullptr;
}

template<typename Connection>
void ConnectionPool<Connection>::ReturnConnection(std::shared_ptr<Connection> conn)
{
    if (!conn) return;
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 创建池化连接对象
    auto pooledConn = std::make_shared<PooledConnection<Connection>>(conn);
    pooledConn->inUse = false;
    pooledConn->lastUsed = std::chrono::steady_clock::now();
    
    // 将连接放回池中
    availableConnections_.push(pooledConn);
    inUseCount_--;
    
    LOG_DEBUG << "Returned connection to pool";
}

template<typename Connection>
void ConnectionPool<Connection>::CleanupExpiredConnections()
{
    auto now = std::chrono::steady_clock::now();
    std::queue<std::shared_ptr<PooledConnection<Connection>>> validConnections;
    
    while (!availableConnections_.empty()) {
        auto pooledConn = availableConnections_.front();
        availableConnections_.pop();
        
        // 检查连接是否过期
        auto idleTime = std::chrono::duration_cast<std::chrono::seconds>(
            now - pooledConn->lastUsed);
        
        if (idleTime < maxIdleTime_) {
            // 未过期，放回有效连接队列
            validConnections.push(pooledConn);
        } else {
            // 过期，关闭连接
            CloseConnection(pooledConn->connection);
            LOG_DEBUG << "Closed expired connection";
        }
    }
    
    // 将有效连接移回主队列
    availableConnections_ = std::move(validConnections);
}