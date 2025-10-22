// src/servicePro/monitor.h
#pragma once

#include <chrono>
#include <atomic>
#include <map>
#include <string>
#include <muduo/base/Logging.h>

// 服务监控指标
class ServiceMonitor
{
public:
    ServiceMonitor(const std::string& serviceName);
    
    // 记录请求
    void RecordRequest(const std::string& method, bool success, int64_t latencyMs);
    
    // 记录错误
    void RecordError(const std::string& method, const std::string& errorType);
    
    // 获取服务统计信息
    void GetStats(std::map<std::string, std::string>& stats);
    
    // 重置统计信息
    void ResetStats();
    
private:
    // 服务名称
    std::string serviceName_;
    
    // 请求计数
    std::atomic<uint64_t> totalRequests_;
    std::atomic<uint64_t> successfulRequests_;
    std::atomic<uint64_t> failedRequests_;
    
    // 延迟统计
    std::atomic<uint64_t> totalLatency_;
    std::atomic<uint64_t> maxLatency_;
    std::atomic<uint64_t> minLatency_;
    
    // 方法级别的统计
    std::map<std::string, std::atomic<uint64_t>> methodRequests_;
    std::map<std::string, std::atomic<uint64_t>> methodSuccess_;
    std::map<std::string, std::atomic<uint64_t>> methodFailures_;
    std::map<std::string, std::atomic<uint64_t>> methodLatency_;
    
    // 错误统计
    std::map<std::string, std::atomic<uint64_t>> errorCounts_;
};