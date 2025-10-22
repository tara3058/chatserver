// src/servicePro/loadbalancer.h
#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <atomic>
#include <muduo/base/Logging.h>

// 负载均衡器基类
class LoadBalancer
{
public:
    virtual ~LoadBalancer() = default;
    
    // 根据用户ID选择服务节点
    virtual std::string SelectNode(int userId) = 0;
    
    // 添加服务节点
    virtual void AddNode(const std::string& node) = 0;
    
    // 移除服务节点
    virtual void RemoveNode(const std::string& node) = 0;
    
    // 获取所有节点
    virtual const std::vector<std::string>& GetNodes() const = 0;
};

// 一致性哈希负载均衡器
class ConsistentHashLoadBalancer : public LoadBalancer
{
public:
    ConsistentHashLoadBalancer();
    
    // 根据用户ID选择服务节点
    std::string SelectNode(int userId) override;
    
    // 添加服务节点
    void AddNode(const std::string& node) override;
    
    // 移除服务节点
    void RemoveNode(const std::string& node) override;
    
    // 获取所有节点
    const std::vector<std::string>& GetNodes() const override { return nodes_; }
    
private:
    // 哈希函数
    uint32_t Hash(const std::string& key);
    
    // 虚拟节点数量
    static const int VIRTUAL_NODE_COUNT = 150;
    
    // 节点列表
    std::vector<std::string> nodes_;
    
    // 虚拟节点到真实节点的映射
    std::map<uint32_t, std::string> virtualNodes_;
};

// 轮询负载均衡器
class RoundRobinLoadBalancer : public LoadBalancer
{
public:
    RoundRobinLoadBalancer();
    
    // 根据用户ID选择服务节点
    std::string SelectNode(int userId) override;
    
    // 添加服务节点
    void AddNode(const std::string& node) override;
    
    // 移除服务节点
    void RemoveNode(const std::string& node) override;
    
    // 获取所有节点
    const std::vector<std::string>& GetNodes() const override { return nodes_; }
    
private:
    std::vector<std::string> nodes_;
    std::atomic<size_t> currentIndex_;
};