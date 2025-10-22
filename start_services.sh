#!/bin/bash

# 启动分布式聊天系统的所有服务

echo "启动分布式聊天系统服务..."

# 创建日志目录
mkdir -p /home/gas/Desktop/project/chat/logs

# 启动用户服务
echo "启动用户服务..."
cd /home/gas/Desktop/project/chat/bin
./user_service 127.0.0.1 8081 > /home/gas/Desktop/project/chat/logs/user_service.log 2>&1 &
USER_SERVICE_PID=$!
echo "用户服务PID: $USER_SERVICE_PID"

# 等待一段时间确保用户服务启动
sleep 2

# 启动消息服务
echo "启动消息服务..."
cd /home/gas/Desktop/project/chat/bin
./message_service 127.0.0.1 8082 > /home/gas/Desktop/project/chat/logs/message_service.log 2>&1 &
MESSAGE_SERVICE_PID=$!
echo "消息服务PID: $MESSAGE_SERVICE_PID"

# 等待一段时间确保消息服务启动
sleep 2

# 启动关系服务
echo "启动关系服务..."
cd /home/gas/Desktop/project/chat/bin
./relation_service 127.0.0.1 8083 > /home/gas/Desktop/project/chat/logs/relation_service.log 2>&1 &
RELATION_SERVICE_PID=$!
echo "关系服务PID: $RELATION_SERVICE_PID"

# 等待一段时间确保关系服务启动
sleep 2

# 启动网关服务
echo "启动网关服务..."
cd /home/gas/Desktop/project/chat/bin
./gateway_service 127.0.0.1 8084 > /home/gas/Desktop/project/chat/logs/gateway_service.log 2>&1 &
GATEWAY_SERVICE_PID=$!
echo "网关服务PID: $GATEWAY_SERVICE_PID"

echo ""
echo "所有服务已启动:"
echo "  用户服务 (PID: $USER_SERVICE_PID) - 端口 8081"
echo "  消息服务 (PID: $MESSAGE_SERVICE_PID) - 端口 8082"
echo "  关系服务 (PID: $RELATION_SERVICE_PID) - 端口 8083"
echo "  网关服务 (PID: $GATEWAY_SERVICE_PID) - 端口 8080"
echo ""
echo "查看日志请使用: tail -f /home/gas/Desktop/project/chat/logs/*.log"
echo "停止服务请使用: ./stop_services.sh"