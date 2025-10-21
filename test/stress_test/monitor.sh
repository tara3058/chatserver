#!/bin/bash

# 聊天服务器性能监控脚本

SERVER_PID=""
LOG_FILE="performance_$(date +%Y%m%d_%H%M%S).log"
MONITOR_DURATION=300  # 监控5分钟

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

echo -e "${GREEN}===== 聊天服务器性能监控 =====${NC}"

# 查找服务器进程
find_server_process() {
    SERVER_PID=$(pgrep -f "chatserver\|ChatServer")
    if [ -z "$SERVER_PID" ]; then
        echo -e "${RED}错误: 未找到聊天服务器进程${NC}"
        echo "请确保服务器正在运行"
        exit 1
    fi
    echo -e "${GREEN}找到服务器进程 PID: $SERVER_PID${NC}"
}

# 监控系统资源
monitor_resources() {
    echo -e "${YELLOW}开始监控系统资源 (持续 ${MONITOR_DURATION} 秒)...${NC}"
    echo "时间戳,CPU使用率(%),内存使用(MB),虚拟内存(MB),文件描述符数,网络连接数,TCP连接数" > $LOG_FILE
    
    for ((i=1; i<=$MONITOR_DURATION; i++)); do
        TIMESTAMP=$(date '+%Y-%m-%d %H:%M:%S')
        
        # CPU使用率
        CPU_USAGE=$(top -bn1 -p $SERVER_PID | tail -1 | awk '{print $9}')
        
        # 内存使用情况 (KB转MB)
        MEMORY_INFO=$(cat /proc/$SERVER_PID/status | grep -E 'VmRSS|VmSize')
        MEMORY_RSS=$(echo "$MEMORY_INFO" | grep VmRSS | awk '{print $2/1024}')
        MEMORY_VSZ=$(echo "$MEMORY_INFO" | grep VmSize | awk '{print $2/1024}')
        
        # 文件描述符数量
        FD_COUNT=$(ls /proc/$SERVER_PID/fd | wc -l)
        
        # 网络连接数 (所有状态)
        NETWORK_CONNECTIONS=$(ss -tan | grep :6000 | wc -l)
        
        # TCP ESTABLISHED 连接数
        TCP_ESTABLISHED=$(ss -tan state established | grep :6000 | wc -l)
        
        echo "$TIMESTAMP,$CPU_USAGE,$MEMORY_RSS,$MEMORY_VSZ,$FD_COUNT,$NETWORK_CONNECTIONS,$TCP_ESTABLISHED" >> $LOG_FILE
        
        # 每10秒显示一次实时信息
        if [ $((i % 10)) -eq 0 ]; then
            echo -e "${BLUE}[$TIMESTAMP]${NC} CPU: ${CPU_USAGE}% | 内存: ${MEMORY_RSS}MB | 连接数: $TCP_ESTABLISHED"
        fi
        
        sleep 1
    done
    
    echo -e "${GREEN}监控完成，日志保存到: $LOG_FILE${NC}"
}

# 生成性能报告
generate_report() {
    echo -e "${YELLOW}生成性能报告...${NC}"
    
    # 计算统计信息
    echo -e "\n${GREEN}===== 性能统计报告 =====${NC}"
    
    # CPU使用率统计
    CPU_AVG=$(awk -F',' 'NR>1 {sum+=$2; count++} END {print sum/count}' $LOG_FILE)
    CPU_MAX=$(awk -F',' 'NR>1 {if($2>max) max=$2} END {print max}' $LOG_FILE)
    echo -e "CPU 使用率 - 平均: ${YELLOW}${CPU_AVG}%${NC}, 最大: ${YELLOW}${CPU_MAX}%${NC}"
    
    # 内存使用统计
    MEM_AVG=$(awk -F',' 'NR>1 {sum+=$3; count++} END {print sum/count}' $LOG_FILE)
    MEM_MAX=$(awk -F',' 'NR>1 {if($3>max) max=$3} END {print max}' $LOG_FILE)
    echo -e "内存使用 - 平均: ${YELLOW}${MEM_AVG}MB${NC}, 最大: ${YELLOW}${MEM_MAX}MB${NC}"
    
    # 连接数统计
    CONN_AVG=$(awk -F',' 'NR>1 {sum+=$7; count++} END {print sum/count}' $LOG_FILE)
    CONN_MAX=$(awk -F',' 'NR>1 {if($7>max) max=$7} END {print max}' $LOG_FILE)
    echo -e "TCP连接数 - 平均: ${YELLOW}${CONN_AVG}${NC}, 最大: ${YELLOW}${CONN_MAX}${NC}"
    
    # 文件描述符统计
    FD_AVG=$(awk -F',' 'NR>1 {sum+=$5; count++} END {print sum/count}' $LOG_FILE)
    FD_MAX=$(awk -F',' 'NR>1 {if($5>max) max=$5} END {print max}' $LOG_FILE)
    echo -e "文件描述符 - 平均: ${YELLOW}${FD_AVG}${NC}, 最大: ${YELLOW}${FD_MAX}${NC}"
}

# 检查系统限制
check_system_limits() {
    echo -e "\n${YELLOW}===== 系统限制检查 =====${NC}"
    
    # 检查最大文件描述符限制
    echo "当前用户最大文件描述符: $(ulimit -n)"
    echo "系统最大文件描述符: $(cat /proc/sys/fs/file-max)"
    
    # 检查TCP连接相关参数
    echo "TCP最大连接数相关参数:"
    echo "  net.core.somaxconn = $(cat /proc/sys/net/core/somaxconn)"
    echo "  net.ipv4.tcp_max_syn_backlog = $(cat /proc/sys/net/ipv4/tcp_max_syn_backlog)"
    echo "  net.core.netdev_max_backlog = $(cat /proc/sys/net/core/netdev_max_backlog)"
    
    # 检查进程限制
    echo "进程限制:"
    echo "  最大进程数: $(ulimit -u)"
    echo "  最大内存锁定: $(ulimit -l)"
}

# 主函数
main() {
    find_server_process
    check_system_limits
    monitor_resources
    generate_report
    
    echo -e "\n${GREEN}性能监控完成!${NC}"
    echo -e "详细数据请查看: ${YELLOW}$LOG_FILE${NC}"
}

# 信号处理
trap 'echo -e "\n${YELLOW}监控被中断${NC}"; exit 1' INT TERM

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--duration)
            MONITOR_DURATION="$2"
            shift 2
            ;;
        -h|--help)
            echo "用法: $0 [-d duration] [-h]"
            echo "  -d, --duration   监控持续时间(秒), 默认300秒"
            echo "  -h, --help       显示帮助信息"
            exit 0
            ;;
        *)
            echo "未知参数: $1"
            exit 1
            ;;
    esac
done

main