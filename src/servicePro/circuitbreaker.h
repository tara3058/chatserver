// src/servicePro/circuitbreaker.h
#pragma once

#include <chrono>
#include <atomic>
#include <muduo/base/Logging.h>

// 熔断器状态
enum class CircuitState {
    CLOSED,    // 关闭状态，正常处理请求
    OPEN,      // 打开状态，拒绝所有请求
    HALF_OPEN  // 半开状态，尝试放行部分请求
};

// 熔断器
class CircuitBreaker
{
public:
    CircuitBreaker(int failureThreshold = 5, 
                   int timeoutMs = 5000, 
                   int halfOpenRetryCount = 3);
    
    // 检查是否允许请求通过
    bool CanPass();
    
    // 记录请求成功
    void OnSuccess();
    
    // 记录请求失败
    void OnFailure();
    
    // 获取当前状态
    CircuitState GetState() const { return state_; }
    
private:
    // 更新状态
    void UpdateState();
    
    // 重置统计信息
    void ResetStats();
    
    // 熔断阈值
    const int failureThreshold_;
    
    // 熔断超时时间（毫秒）
    const int timeoutMs_;
    
    // 半开状态下允许的最大重试次数
    const int maxHalfOpenRetryCount_;
    
    // 当前状态
    std::atomic<CircuitState> state_;
    
    // 失败计数
    std::atomic<int> failureCount_;
    
    // 成功计数
    std::atomic<int> successCount_;
    
    // 最后一次失败时间
    std::chrono::steady_clock::time_point lastFailureTime_;
    
    // 半开状态下的尝试次数
    std::atomic<int> halfOpenRetryCount_;
};