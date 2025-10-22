// src/servicePro/circuitbreaker.cc
#include "circuitbreaker.h"

CircuitBreaker::CircuitBreaker(int failureThreshold, 
                               int timeoutMs, 
                               int halfOpenRetryCount)
    : failureThreshold_(failureThreshold)
    , timeoutMs_(timeoutMs)
    , maxHalfOpenRetryCount_(halfOpenRetryCount)
    , state_(CircuitState::CLOSED)
    , failureCount_(0)
    , successCount_(0)
    , lastFailureTime_(std::chrono::steady_clock::now())
    , halfOpenRetryCount_(0)
{
    LOG_INFO << "CircuitBreaker created with failureThreshold=" << failureThreshold
             << ", timeoutMs=" << timeoutMs << ", halfOpenRetryCount=" << halfOpenRetryCount;
}

bool CircuitBreaker::CanPass()
{
    switch (state_.load()) {
        case CircuitState::CLOSED:
            return true;
            
        case CircuitState::OPEN:
            {
                auto now = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - lastFailureTime_).count();
                
                // 如果超时时间已到，进入半开状态
                if (elapsed >= timeoutMs_) {
                    state_ = CircuitState::HALF_OPEN;
                    halfOpenRetryCount_ = 0;
                    LOG_INFO << "Circuit breaker state changed to HALF_OPEN";
                    return true;
                }
                
                LOG_DEBUG << "Circuit breaker is OPEN, request rejected";
                return false;
            }
            
        case CircuitState::HALF_OPEN:
            // 在半开状态下，限制尝试次数
            if (halfOpenRetryCount_.load() < maxHalfOpenRetryCount_) {
                halfOpenRetryCount_++;
                return true;
            }
            return false;
            
        default:
            return false;
    }
}

void CircuitBreaker::OnSuccess()
{
    successCount_++;
    
    if (state_.load() == CircuitState::HALF_OPEN) {
        // 在半开状态下，如果成功次数达到阈值，则关闭熔断器
        if (successCount_.load() >= maxHalfOpenRetryCount_) {
            state_ = CircuitState::CLOSED;
            ResetStats();
            LOG_INFO << "Circuit breaker state changed to CLOSED";
        }
    }
}

void CircuitBreaker::OnFailure()
{
    failureCount_++;
    lastFailureTime_ = std::chrono::steady_clock::now();
    
    switch (state_.load()) {
        case CircuitState::CLOSED:
            // 如果失败次数达到阈值，则打开熔断器
            if (failureCount_.load() >= failureThreshold_) {
                state_ = CircuitState::OPEN;
                LOG_INFO << "Circuit breaker state changed to OPEN";
            }
            break;
            
        case CircuitState::HALF_OPEN:
            // 在半开状态下，如果失败，则重新打开熔断器
            state_ = CircuitState::OPEN;
            LOG_INFO << "Circuit breaker state changed to OPEN from HALF_OPEN";
            break;
            
        default:
            break;
    }
}

void CircuitBreaker::ResetStats()
{
    failureCount_ = 0;
    successCount_ = 0;
    halfOpenRetryCount_ = 0;
}