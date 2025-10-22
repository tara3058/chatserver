// src/servicePro/loadbalancer.cc
#include "loadbalancer.h"
#include <functional>
#include <algorithm>
#include <atomic>

// 一致性哈希负载均衡器实现
ConsistentHashLoadBalancer::ConsistentHashLoadBalancer()
{
    LOG_INFO << "ConsistentHashLoadBalancer created";
}

void ConsistentHashLoadBalancer::AddNode(const std::string& node)
{
    nodes_.push_back(node);
    
    // 为每个真实节点创建虚拟节点
    for (int i = 0; i < VIRTUAL_NODE_COUNT; ++i) {
        std::string virtualNodeKey = node + "&&VN" + std::to_string(i);
        uint32_t hash = Hash(virtualNodeKey);
        virtualNodes_[hash] = node;
    }
    
    LOG_INFO << "Added node: " << node;
}

void ConsistentHashLoadBalancer::RemoveNode(const std::string& node)
{
    // 从节点列表中移除
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node), nodes_.end());
    
    // 移除对应的虚拟节点
    for (auto it = virtualNodes_.begin(); it != virtualNodes_.end();) {
        if (it->second == node) {
            it = virtualNodes_.erase(it);
        } else {
            ++it;
        }
    }
    
    LOG_INFO << "Removed node: " << node;
}

std::string ConsistentHashLoadBalancer::SelectNode(int userId)
{
    if (virtualNodes_.empty()) {
        LOG_WARN << "No nodes available for load balancing";
        return "";
    }
    
    // 计算用户ID的哈希值
    std::string key = std::to_string(userId);
    uint32_t hash = Hash(key);
    
    // 找到第一个大于等于该哈希值的虚拟节点
    auto it = virtualNodes_.lower_bound(hash);
    if (it == virtualNodes_.end()) {
        // 如果没有找到，则选择第一个节点
        it = virtualNodes_.begin();
    }
    
    LOG_DEBUG << "Selected node " << it->second << " for user " << userId;
    return it->second;
}

uint32_t ConsistentHashLoadBalancer::Hash(const std::string& key)
{
    // 简单的哈希函数实现
    uint32_t hash = 0;
    for (char c : key) {
        hash = hash * 31 + c;
    }
    return hash;
}

// 轮询负载均衡器实现
RoundRobinLoadBalancer::RoundRobinLoadBalancer() : currentIndex_(0)
{
    LOG_INFO << "RoundRobinLoadBalancer created";
}

void RoundRobinLoadBalancer::AddNode(const std::string& node)
{
    nodes_.push_back(node);
    LOG_INFO << "Added node: " << node;
}

void RoundRobinLoadBalancer::RemoveNode(const std::string& node)
{
    nodes_.erase(std::remove(nodes_.begin(), nodes_.end(), node), nodes_.end());
    LOG_INFO << "Removed node: " << node;
}

std::string RoundRobinLoadBalancer::SelectNode(int userId)
{
    if (nodes_.empty()) {
        LOG_WARN << "No nodes available for load balancing";
        return "";
    }
    
    // 使用用户ID作为种子来选择节点，确保同一用户总是路由到同一节点
    size_t index = userId % nodes_.size();
    LOG_DEBUG << "Selected node " << nodes_[index] << " for user " << userId;
    return nodes_[index];
}