# 聊天服务器压测工具使用指南

本目录包含了一套完整的聊天服务器压测工具，支持多种压测方式和详细的性能监控。

## 目录结构

```
test/stress_test/
├── stress_client.cpp          # 主压测客户端源码
├── connection_test.cpp        # 简单连接测试源码
├── CMakeLists.txt            # 构建配置
├── run_stress_test.sh        # 综合压测脚本 (推荐)
├── monitor.sh                # 性能监控脚本
├── tcpkali_test.sh          # tcpkali压测脚本
└── README.md                # 本文档
```

## 快速开始

### 1. 启动聊天服务器

```bash
cd /home/gas/Desktop/project/chat
mkdir -p build && cd build
cmake .. && make
cd ../bin
./chatserver 127.0.0.1 6000
```

### 2. 运行综合压测 (推荐)

```bash
cd /home/gas/Desktop/project/chat/test/stress_test
./run_stress_test.sh
```

这将自动执行以下步骤：
- 编译压测工具
- 连接测试
- 预热测试 (10个客户端，30秒)
- 多级压力测试 (50/100/200/500个客户端)
- 性能监控和报告生成

## 详细使用说明

### 方案一：自定义C++压测客户端

#### 编译
```bash
cd test/stress_test
mkdir build && cd build
cmake .. && make
```

#### 运行连接测试
```bash
./connection_test 127.0.0.1 6000
```

#### 运行压力测试
```bash
# 语法: ./stress_client <server_ip> <server_port> <client_count> <duration>
./stress_client 127.0.0.1 6000 100 60
```

参数说明：
- `server_ip`: 服务器IP地址
- `server_port`: 服务器端口
- `client_count`: 并发客户端数量 (1-10000)
- `duration`: 测试时长(秒) (1-3600)

### 方案二：性能监控

#### 启动监控
```bash
./monitor.sh -d 300  # 监控300秒
```

监控内容：
- CPU使用率
- 内存使用量
- 文件描述符数量
- 网络连接数
- TCP ESTABLISHED连接数

#### 监控报告
监控完成后会生成：
- CSV格式的详细数据文件
- 统计摘要报告

### 方案三：tcpkali压测

#### 安装tcpkali
```bash
sudo apt-get install tcpkali
```

#### 运行tcpkali压测
```bash
./tcpkali_test.sh
```

可以修改脚本中的参数：
- `CONNECTIONS`: 并发连接数
- `DURATION`: 测试时长
- `MESSAGE_RATE`: 消息发送频率

## 压测场景

### 1. 基础连接测试
测试服务器是否能正常接受连接和处理消息。

### 2. 并发连接测试
测试服务器在不同并发连接数下的表现：
- 50个并发连接
- 100个并发连接  
- 200个并发连接
- 500个并发连接

### 3. 消息吞吐量测试
测试服务器处理消息的能力：
- 登录消息处理
- 聊天消息转发
- 离线消息存储

### 4. 长连接稳定性测试
测试服务器长时间运行的稳定性。

## 性能指标

### 关键指标
- **连接成功率**: 成功建立连接的客户端比例
- **消息吞吐量**: 每秒处理的消息数量  
- **响应延迟**: 消息从发送到接收的时间
- **资源使用**: CPU、内存、文件描述符使用情况

### 预期性能基准
基于Muduo网络库的特性，预期指标：
- 单机支持 10,000+ 并发连接
- 消息处理延迟 < 10ms
- CPU使用率 < 80%
- 内存使用稳定增长

## 故障排查

### 常见问题

1. **编译失败**
   ```
   错误: muduo库未找到
   解决: 安装muduo开发库或检查库路径
   ```

2. **连接被拒绝**
   ```
   错误: Connection refused
   解决: 检查服务器是否运行，端口是否正确
   ```

3. **文件描述符不足**
   ```
   错误: Too many open files
   解决: 增加系统限制 ulimit -n 65536
   ```

4. **内存不足**
   ```
   错误: Cannot allocate memory
   解决: 减少并发连接数或增加系统内存
   ```

### 系统优化建议

#### 操作系统参数
```bash
# 增加文件描述符限制
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf

# 网络参数优化
echo "net.core.somaxconn = 65536" >> /etc/sysctl.conf
echo "net.ipv4.tcp_max_syn_backlog = 65536" >> /etc/sysctl.conf
echo "net.core.netdev_max_backlog = 5000" >> /etc/sysctl.conf
sysctl -p
```

#### 数据库优化
- 增加MySQL连接池大小
- 优化数据库查询索引
- 考虑读写分离

#### Redis优化  
- 配置适当的内存限制
- 启用持久化策略
- 考虑Redis集群

## 压测报告示例

```
===== 压测结果摘要 =====
测试时间: 2024-01-01 10:00:00
并发连接数: 500
测试时长: 120秒
连接成功率: 99.8%
消息总数: 60,000
平均QPS: 500
平均延迟: 8.5ms
CPU峰值: 75%
内存峰值: 512MB
```

## 持续集成

可以将压测集成到CI/CD流程中：

```bash
# 在部署后自动执行压测
./run_stress_test.sh --clients 100 --duration 60
```

## 扩展功能

### 自定义消息类型
修改 `stress_client.cpp` 中的消息生成函数，支持：
- 群聊消息测试
- 好友操作测试  
- 文件传输测试

### 分布式压测
在多台机器上运行压测客户端，模拟更大规模的并发。

### 实时监控
集成Grafana + InfluxDB，实现实时性能监控。

## 联系与支持

如有问题，请检查：
1. 服务器日志
2. 系统资源限制
3. 网络连接状态
4. 数据库连接状态