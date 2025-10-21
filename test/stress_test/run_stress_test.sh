#!/bin/bash

# 聊天服务器综合压测脚本

# 配置参数
SERVER_IP="127.0.0.1"
SERVER_PORT="6000"
PROJECT_ROOT="/home/gas/Desktop/project/chat"
BUILD_DIR="$PROJECT_ROOT/test/stress_test/build"
BIN_DIR="$PROJECT_ROOT/bin"

# 压测参数
WARMUP_CLIENTS=10
WARMUP_DURATION=30
STRESS_CLIENTS_LEVELS=(50 100 200 500)
STRESS_DURATION=120

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 日志文件
RESULT_FILE="stress_test_results_$(date +%Y%m%d_%H%M%S).txt"

echo -e "${GREEN}===== 聊天服务器综合压测 =====${NC}"

# 检查依赖
check_dependencies() {
    echo -e "${YELLOW}检查依赖...${NC}"
    
    # 检查muduo库
    if ! ldconfig -p | grep -q muduo; then
        echo -e "${RED}警告: muduo库可能未正确安装${NC}"
    fi
    
    # 检查服务器是否运行
    if ! nc -z $SERVER_IP $SERVER_PORT; then
        echo -e "${RED}错误: 聊天服务器未运行 (${SERVER_IP}:${SERVER_PORT})${NC}"
        echo "请先启动聊天服务器: cd $BIN_DIR && ./chatserver $SERVER_IP $SERVER_PORT"
        exit 1
    fi
    
    echo -e "${GREEN}依赖检查通过${NC}"
}

# 编译压测工具
build_stress_tools() {
    echo -e "${YELLOW}编译压测工具...${NC}"
    
    cd "$PROJECT_ROOT/test/stress_test"
    
    if [ ! -d "build" ]; then
        mkdir build
    fi
    
    cd build
    
    if cmake .. && make; then
        echo -e "${GREEN}压测工具编译成功${NC}"
    else
        echo -e "${RED}编译失败${NC}"
        exit 1
    fi
}

# 执行连接测试
run_connection_test() {
    echo -e "${YELLOW}执行连接测试...${NC}"
    
    cd "$BUILD_DIR"
    
    if [ -f "connection_test" ]; then
        echo -e "${BLUE}运行简单连接测试...${NC}"
        ./connection_test $SERVER_IP $SERVER_PORT
        
        if [ $? -eq 0 ]; then
            echo -e "${GREEN}连接测试通过${NC}"
        else
            echo -e "${RED}连接测试失败${NC}"
            exit 1
        fi
    else
        echo -e "${RED}connection_test 可执行文件不存在${NC}"
        exit 1
    fi
}

# 预热测试
warmup_test() {
    echo -e "${YELLOW}执行预热测试...${NC}"
    echo "预热参数: ${WARMUP_CLIENTS}个客户端, ${WARMUP_DURATION}秒"
    
    cd "$BUILD_DIR"
    
    ./stress_client $SERVER_IP $SERVER_PORT $WARMUP_CLIENTS $WARMUP_DURATION
    
    echo -e "${GREEN}预热测试完成，等待5秒...${NC}"
    sleep 5
}

# 压力测试
stress_test() {
    echo -e "${YELLOW}开始压力测试...${NC}"
    
    cd "$BUILD_DIR"
    
    echo "测试时间: $(date)" >> "$PROJECT_ROOT/$RESULT_FILE"
    echo "服务器: $SERVER_IP:$SERVER_PORT" >> "$PROJECT_ROOT/$RESULT_FILE"
    echo "=================================" >> "$PROJECT_ROOT/$RESULT_FILE"
    
    for clients in "${STRESS_CLIENTS_LEVELS[@]}"; do
        echo -e "${BLUE}测试 $clients 个并发客户端...${NC}"
        echo "" >> "$PROJECT_ROOT/$RESULT_FILE"
        echo "=== $clients 个客户端测试 ===" >> "$PROJECT_ROOT/$RESULT_FILE"
        
        # 启动性能监控
        echo -e "${YELLOW}启动性能监控...${NC}"
        bash "$PROJECT_ROOT/test/stress_test/monitor.sh" -d $STRESS_DURATION &
        MONITOR_PID=$!
        
        # 等待1秒让监控启动
        sleep 1
        
        # 执行压测
        echo "开始时间: $(date)" >> "$PROJECT_ROOT/$RESULT_FILE"
        ./stress_client $SERVER_IP $SERVER_PORT $clients $STRESS_DURATION 2>&1 | tee -a "$PROJECT_ROOT/$RESULT_FILE"
        echo "结束时间: $(date)" >> "$PROJECT_ROOT/$RESULT_FILE"
        
        # 等待监控结束
        wait $MONITOR_PID
        
        echo -e "${BLUE}$clients 客户端测试完成，等待10秒后继续...${NC}"
        sleep 10
    done
    
    echo -e "${GREEN}所有压力测试完成！${NC}"
}

# 生成最终报告
generate_final_report() {
    echo -e "${YELLOW}生成最终报告...${NC}"
    
    REPORT_FILE="final_report_$(date +%Y%m%d_%H%M%S).md"
    
    cat > "$PROJECT_ROOT/$REPORT_FILE" << EOF
# 聊天服务器压测报告

## 测试环境
- 服务器地址: $SERVER_IP:$SERVER_PORT
- 测试时间: $(date)
- 操作系统: $(uname -a)
- CPU信息: $(cat /proc/cpuinfo | grep "model name" | head -1 | cut -d: -f2 | xargs)
- 内存信息: $(free -h | grep Mem | awk '{print $2}')

## 测试配置
- 预热测试: ${WARMUP_CLIENTS}个客户端, ${WARMUP_DURATION}秒
- 压力测试: ${STRESS_CLIENTS_LEVELS[*]}个客户端级别
- 每级测试时长: ${STRESS_DURATION}秒

## 测试结果
详细结果请查看: $RESULT_FILE

## 系统配置建议
基于测试结果，建议检查以下系统参数：
- ulimit -n (文件描述符限制)
- net.core.somaxconn
- net.ipv4.tcp_max_syn_backlog
- 数据库连接池配置
- Redis连接配置

## 性能优化建议
1. 增加数据库连接池大小
2. 优化Redis使用策略
3. 调整Muduo网络库线程数
4. 考虑负载均衡方案
EOF

    echo -e "${GREEN}最终报告生成: $REPORT_FILE${NC}"
}

# 清理函数
cleanup() {
    echo -e "${YELLOW}清理资源...${NC}"
    # 杀死可能残留的进程
    pkill -f stress_client 2>/dev/null
    pkill -f connection_test 2>/dev/null
}

# 信号处理
trap cleanup EXIT INT TERM

# 主函数
main() {
    echo -e "${BLUE}压测配置:${NC}"
    echo "  服务器: $SERVER_IP:$SERVER_PORT"
    echo "  预热: ${WARMUP_CLIENTS}客户端 × ${WARMUP_DURATION}秒"
    echo "  压测级别: ${STRESS_CLIENTS_LEVELS[*]}"
    echo "  每级时长: ${STRESS_DURATION}秒"
    echo ""
    
    read -p "按Enter开始压测，或Ctrl+C取消..." 
    
    check_dependencies
    build_stress_tools
    run_connection_test
    warmup_test
    stress_test
    generate_final_report
    
    echo -e "\n${GREEN}=== 压测完成 ===${NC}"
    echo -e "结果文件: ${YELLOW}$PROJECT_ROOT/$RESULT_FILE${NC}"
    echo -e "报告文件: ${YELLOW}$PROJECT_ROOT/$REPORT_FILE${NC}"
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        --server)
            SERVER_IP="$2"
            shift 2
            ;;
        --port)
            SERVER_PORT="$2"
            shift 2
            ;;
        --clients)
            IFS=',' read -ra STRESS_CLIENTS_LEVELS <<< "$2"
            shift 2
            ;;
        --duration)
            STRESS_DURATION="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [选项]"
            echo "选项:"
            echo "  --server IP      服务器IP (默认: 127.0.0.1)"
            echo "  --port PORT      服务器端口 (默认: 6000)"
            echo "  --clients N,M,L  客户端数量级别 (默认: 50,100,200,500)"
            echo "  --duration SEC   每级测试时长 (默认: 120)"
            echo "  -h, --help       显示帮助"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

main