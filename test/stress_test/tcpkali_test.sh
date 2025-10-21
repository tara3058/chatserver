#!/bin/bash

# Chat Server 压测脚本
# 使用tcpkali工具进行TCP连接压测

SERVER_IP="127.0.0.1"
SERVER_PORT="6000"
CONNECTIONS=100
DURATION=60
MESSAGE_RATE=10

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}===== Chat Server 压测脚本 =====${NC}"
echo -e "${YELLOW}服务器地址: ${SERVER_IP}:${SERVER_PORT}${NC}"
echo -e "${YELLOW}并发连接数: ${CONNECTIONS}${NC}"
echo -e "${YELLOW}测试时长: ${DURATION}秒${NC}"
echo -e "${YELLOW}消息发送频率: ${MESSAGE_RATE}/秒${NC}"

# 检查tcpkali是否安装
if ! command -v tcpkali &> /dev/null; then
    echo -e "${RED}错误: tcpkali 未安装${NC}"
    echo "在Ubuntu上安装: sudo apt-get install tcpkali"
    echo "或从源码编译: https://github.com/machinezone/tcpkali"
    exit 1
fi

# 检查服务器是否可达
echo -e "${YELLOW}检查服务器连接...${NC}"
if ! nc -z $SERVER_IP $SERVER_PORT; then
    echo -e "${RED}错误: 无法连接到服务器 ${SERVER_IP}:${SERVER_PORT}${NC}"
    echo "请确保聊天服务器正在运行"
    exit 1
fi

echo -e "${GREEN}服务器连接正常${NC}"

# 创建消息模板
LOGIN_MSG='{"msgid":1,"id":1001,"password":"123456"}'
CHAT_MSG='{"msgid":5,"id":1001,"from":1001,"to":1002,"msg":"压测消息","time":"1640995200"}'

echo -e "${YELLOW}开始压测...${NC}"

# 执行压测
tcpkali \
    --connections $CONNECTIONS \
    --duration ${DURATION}s \
    --message-rate $MESSAGE_RATE \
    --message "$LOGIN_MSG" \
    --message "$CHAT_MSG" \
    --latency-percentiles 50,95,99 \
    --statsd \
    $SERVER_IP:$SERVER_PORT

echo -e "${GREEN}压测完成!${NC}"

# 显示系统资源使用情况
echo -e "${YELLOW}=== 系统资源使用情况 ===${NC}"
echo "CPU使用率:"
top -bn1 | grep "Cpu(s)" | awk '{print $2}' | awk -F'%' '{print $1}'
echo "内存使用情况:"
free -h
echo "网络连接数:"
ss -tan | grep :$SERVER_PORT | wc -l