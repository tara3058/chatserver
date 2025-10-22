// src/servicePro/monitor.cc
#include "monitor.h"
#include <sstream>

ServiceMonitor::ServiceMonitor(const std::string& serviceName)
    : serviceName_(serviceName)
    , totalRequests_(0)
    , successfulRequests_(0)
    , failedRequests_(0)
    , totalLatency_(0)
    , maxLatency_(0)
    , minLatency_(UINT64_MAX)
{
    LOG_INFO << "ServiceMonitor created for " << serviceName;
}

void ServiceMonitor::RecordRequest(const std::string& method, bool success, int64_t latencyMs)
{
    totalRequests_++;
    totalLatency_ += latencyMs;
    
    // 更新最大和最小延迟
    if (latencyMs > static_cast<int64_t>(maxLatency_.load())) {
        maxLatency_ = latencyMs;
    }
    if (latencyMs < static_cast<int64_t>(minLatency_.load()) && latencyMs > 0) {
        minLatency_ = latencyMs;
    }
    
    // 方法级别的统计
    methodRequests_[method]++;
    methodLatency_[method] += latencyMs;
    
    if (success) {
        successfulRequests_++;
        methodSuccess_[method]++;
    } else {
        failedRequests_++;
        methodFailures_[method]++;
    }
}

void ServiceMonitor::RecordError(const std::string& method, const std::string& errorType)
{
    errorCounts_[errorType]++;
    LOG_ERROR << "Service " << serviceName_ << " method " << method 
              << " encountered error: " << errorType;
}

void ServiceMonitor::GetStats(std::map<std::string, std::string>& stats)
{
    stats["service_name"] = serviceName_;
    stats["total_requests"] = std::to_string(totalRequests_.load());
    stats["successful_requests"] = std::to_string(successfulRequests_.load());
    stats["failed_requests"] = std::to_string(failedRequests_.load());
    
    uint64_t totalRequests = totalRequests_.load();
    if (totalRequests > 0) {
        double avgLatency = static_cast<double>(totalLatency_.load()) / totalRequests;
        stats["average_latency_ms"] = std::to_string(avgLatency);
    } else {
        stats["average_latency_ms"] = "0";
    }
    
    stats["max_latency_ms"] = std::to_string(maxLatency_.load());
    stats["min_latency_ms"] = std::to_string(minLatency_.load() == UINT64_MAX ? 0 : minLatency_.load());
    
    // 添加方法级别的统计
    for (const auto& pair : methodRequests_) {
        const std::string& method = pair.first;
        std::string prefix = "method_" + method + "_";
        
        stats[prefix + "requests"] = std::to_string(pair.second.load());
        stats[prefix + "success"] = std::to_string(methodSuccess_[method].load());
        stats[prefix + "failures"] = std::to_string(methodFailures_[method].load());
        
        uint64_t requests = pair.second.load();
        if (requests > 0) {
            double avgLatency = static_cast<double>(methodLatency_[method].load()) / requests;
            stats[prefix + "avg_latency_ms"] = std::to_string(avgLatency);
        }
    }
    
    // 添加错误统计
    for (const auto& pair : errorCounts_) {
        stats["error_" + pair.first] = std::to_string(pair.second.load());
    }
}

void ServiceMonitor::ResetStats()
{
    totalRequests_ = 0;
    successfulRequests_ = 0;
    failedRequests_ = 0;
    totalLatency_ = 0;
    maxLatency_ = 0;
    minLatency_ = UINT64_MAX;
    
    for (auto& pair : methodRequests_) {
        pair.second = 0;
    }
    
    for (auto& pair : methodSuccess_) {
        pair.second = 0;
    }
    
    for (auto& pair : methodFailures_) {
        pair.second = 0;
    }
    
    for (auto& pair : methodLatency_) {
        pair.second = 0;
    }
    
    for (auto& pair : errorCounts_) {
        pair.second = 0;
    }
    
    LOG_INFO << "ServiceMonitor stats reset for " << serviceName_;
}