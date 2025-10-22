#!/bin/bash

# 停止分布式聊天系统的所有服务

echo "停止分布式聊天系统服务..."

# 查找并停止所有相关的服务进程
pids=$(pgrep -f "(user_service|message_service|relation_service|gateway_service)")
if [ -n "$pids" ]; then
    echo "找到以下服务进程，正在停止..."
    for pid in $pids; do
        echo "  停止进程 PID: $pid"
        kill $pid
    done
    
    # 等待进程结束
    sleep 2
    
    # 强制杀死仍未结束的进程
    pids=$(pgrep -f "(user_service|message_service|relation_service|gateway_service)")
    if [ -n "$pids" ]; then
        echo "强制终止以下进程..."
        for pid in $pids; do
            echo "  强制终止进程 PID: $pid"
            kill -9 $pid
        done
    fi
else
    echo "未找到正在运行的服务进程"
fi

echo "服务停止完成"