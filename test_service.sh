#!/bin/bash

# 测试服务是否正常运行的脚本

echo "测试分布式聊天系统服务..."

# 检查必需的命令
for cmd in nc telnet; do
    if ! command -v $cmd &> /dev/null; then
        echo "警告: $cmd 未安装，将跳过相关测试"
    fi
done

# 测试端口是否开放
test_port() {
    local host=$1
    local port=$2
    local service=$3
    
    if nc -z $host $port 2>/dev/null; then
        echo "✓ $service (端口 $port) 正在运行"
        return 0
    else
        echo "✗ $service (端口 $port) 未运行"
        return 1
    fi
}

# 测试所有服务端口
echo "检查服务端口..."
test_port 127.0.0.1 8080 "网关服务"
test_port 127.0.0.1 8081 "用户服务"
test_port 127.0.0.1 8082 "消息服务"
test_port 127.0.0.1 8083 "关系服务"
test_port 127.0.0.1 2181 "ZooKeeper"

echo ""
echo "服务状态检查完成。请确保所有必需服务都显示为运行状态。"

echo ""
echo "要进行功能测试，请执行以下操作："
echo "1. 打开一个新的终端窗口"
echo "2. 运行客户端程序: cd /home/gas/Desktop/project/chat/bin && ./client"
echo "3. 在客户端中尝试注册新用户:"
echo "   {\"msgid\":2,\"name\":\"testuser\",\"password\":\"testpass\"}"
echo "4. 登录:"
echo "   {\"msgid\":1,\"id\":1,\"password\":\"testpass\"}"
echo "5. 发送消息给其他用户:"
echo "   {\"msgid\":6,\"id\":1,\"from\":\"testuser\",\"to\":2,\"msg\":\"Hello World\"}"