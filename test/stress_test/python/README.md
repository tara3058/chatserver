# 聊天服务器压测工具 - Python版本

## 概述

这是一套完整的Python版本聊天服务器压测工具，基于asyncio实现高并发TCP连接和消息收发，提供了比C++版本更简洁易用的压测方案。

## 功能特性

- ✅ **高并发压测**: 基于asyncio异步框架，支持数千并发连接
- ✅ **实时监控**: 详细的CPU、内存、网络连接监控
- ✅ **多种测试模式**: 连接测试、压力测试、性能监控
- ✅ **详细报告**: 自动生成性能分析报告和图表数据
- ✅ **易于扩展**: Python代码结构清晰，便于修改和扩展

## 文件结构

```
python/
├── stress_client.py          # 主压测客户端
├── connection_test.py        # 连接功能测试
├── monitor.py               # 性能监控工具
├── run_stress_test.py       # 综合压测管理器
├── quick_test.py            # 快速测试工具
├── requirements.txt         # Python依赖
└── README.md               # 本文档
```

## 安装依赖

```bash
# 安装Python依赖
pip install psutil

# 或使用requirements.txt
pip install -r requirements.txt
```

## 使用方法

### 1. 快速开始

```bash
# 快速测试 (推荐新手使用)
python quick_test.py quick 127.0.0.1 6000

# 或者分步执行
python quick_test.py test 127.0.0.1 6000    # 连接测试
python quick_test.py stress 127.0.0.1 6000 50 60  # 压力测试
python quick_test.py monitor 120             # 性能监控
```

### 2. 单独工具使用

#### 连接功能测试
```bash
python connection_test.py 127.0.0.1 6000
```

验证服务器的基本功能：
- TCP连接建立
- 用户登录
- 聊天消息发送
- 群聊消息发送

#### 压力测试
```bash
# 基本用法: python stress_client.py <host> <port> <clients> <duration>
python stress_client.py 127.0.0.1 6000 100 60

# 参数说明:
# host     - 服务器IP地址
# port     - 服务器端口
# clients  - 并发客户端数量 (1-10000)
# duration - 测试时长(秒) (1-3600)

# 高级参数
python stress_client.py 127.0.0.1 6000 100 60 --interval 0.05 --log-level DEBUG
```

#### 性能监控
```bash
python monitor.py --duration 300 --interval 1.0

# 参数说明:
# --duration  监控时长(秒)
# --interval  采样间隔(秒)
# --process   监控的进程名 (默认: chatserver)
# --port      监控的端口 (默认: 6000)
```

### 3. 综合压测管理器

```bash
# 完整的自动化压测流程
python run_stress_test.py

# 自定义参数
python run_stress_test.py \
    --server 127.0.0.1 \
    --port 6000 \
    --clients 10,50,100,200 \
    --duration 120 \
    --warmup-clients 5 \
    --warmup-duration 30
```

压测流程包括：
1. 依赖检查
2. 连接功能测试
3. 预热测试
4. 多级压力测试
5. 性能监控
6. 生成汇总报告

## 压测场景

### 基础压测方案
```bash
# 1. 小规模验证 (开发环境)
python stress_client.py 127.0.0.1 6000 10 30

# 2. 中等规模测试 (测试环境)
python stress_client.py 127.0.0.1 6000 100 120

# 3. 大规模压测 (生产环境)
python stress_client.py 127.0.0.1 6000 500 300
```

### 进阶压测方案
```bash
# 梯度压测 - 自动执行多个级别
python run_stress_test.py --clients 50,100,200,500,1000 --duration 180

# 长时间稳定性测试
python stress_client.py 127.0.0.1 6000 200 3600

# 高频消息测试
python stress_client.py 127.0.0.1 6000 100 120 --interval 0.01
```

## 性能指标解读

### 关键指标
- **连接成功率**: 应该 > 99%
- **登录成功率**: 应该 > 95%
- **消息QPS**: 取决于服务器性能和业务逻辑
- **平均延迟**: 通常 < 50ms
- **CPU使用率**: 建议 < 80%
- **内存使用**: 监控内存泄漏

### 报告示例
```
===== 压力测试报告 =====
测试配置:
  服务器: 127.0.0.1:6000
  客户端数量: 100
  测试时长: 60秒

连接统计:
  连接成功率: 100/100 (100.0%)
  登录成功率: 98/100 (98.0%)

消息统计:
  总发送消息数: 58234
  总接收消息数: 56891
  平均QPS: 970.57
  每客户端平均发送: 582.3

连接时间统计:
  平均连接时间: 0.023秒
  最大连接时间: 0.156秒
```

## 扩展开发

### 自定义消息类型
修改 `stress_client.py` 中的消息生成函数：

```python
async def send_custom_message(self):
    """发送自定义消息"""
    custom_msg = {
        "msgid": 10,  # 自定义消息类型
        "id": self.user_id,
        "data": "自定义数据",
        "timestamp": int(time.time())
    }
    return await self.send_message(custom_msg)
```

### 添加新的监控指标
修改 `monitor.py` 中的 `PerformanceMetrics` 类：

```python
@dataclass
class PerformanceMetrics:
    # ... 现有字段 ...
    custom_metric: float = 0  # 新增指标
```

### 集成其他测试工具
可以将Python压测工具集成到：
- CI/CD流水线
- 自动化测试框架
- 性能监控系统
- 报警系统

## 最佳实践

### 1. 压测前准备
```bash
# 检查系统限制
ulimit -n 65536

# 启动服务器
cd /path/to/chat/bin
./chatserver 127.0.0.1 6000

# 检查服务器状态
python connection_test.py 127.0.0.1 6000
```

### 2. 分阶段压测
```bash
# 阶段1: 功能验证
python quick_test.py test 127.0.0.1 6000

# 阶段2: 小规模压测
python stress_client.py 127.0.0.1 6000 50 60

# 阶段3: 逐步增加负载
python run_stress_test.py --clients 50,100,200,500
```

### 3. 监控和分析
```bash
# 同时运行监控
python monitor.py --duration 300 &
python stress_client.py 127.0.0.1 6000 200 300

# 分析结果
ls performance_monitor_*.csv
ls stress_test_*.log
```

## 故障排查

### 常见问题

1. **连接被拒绝**
   ```
   ConnectionRefusedError: [Errno 111] Connection refused
   ```
   解决：检查服务器是否启动，端口是否正确

2. **文件描述符不足**
   ```
   OSError: [Errno 24] Too many open files
   ```
   解决：`ulimit -n 65536`

3. **内存不足**
   ```
   MemoryError: Unable to allocate memory
   ```
   解决：减少并发数或增加系统内存

4. **JSON解析错误**
   ```
   json.JSONDecodeError: Expecting value
   ```
   解决：检查服务器响应格式，可能需要调整消息解析逻辑

### 调试技巧

```bash
# 开启详细日志
python stress_client.py 127.0.0.1 6000 10 30 --log-level DEBUG

# 监控系统资源
htop
iostat 1
netstat -tuln | grep 6000

# 查看服务器日志
tail -f /path/to/server.log
```

## 性能调优

### 客户端优化
- 调整消息发送间隔
- 优化连接复用
- 使用连接池

### 服务器优化
- 调整Muduo线程数
- 优化数据库连接池
- 启用Redis缓存
- 调整TCP内核参数

### 系统优化
```bash
# 网络参数优化
echo 65536 > /proc/sys/net/core/somaxconn
echo 65536 > /proc/sys/net/ipv4/tcp_max_syn_backlog

# 文件描述符优化
echo "* soft nofile 65536" >> /etc/security/limits.conf
echo "* hard nofile 65536" >> /etc/security/limits.conf
```

## 与C++版本对比

| 特性 | Python版本 | C++版本 |
|------|-----------|---------|
| 开发难度 | 简单 | 复杂 |
| 运行性能 | 中等 | 高 |
| 可扩展性 | 强 | 中等 |
| 依赖管理 | 简单 | 复杂 |
| 跨平台性 | 好 | 一般 |
| 调试便利性 | 高 | 中等 |

## 总结

Python版本的压测工具提供了更加简洁易用的压测方案，特别适合：

- 快速验证服务器功能
- 开发阶段的性能测试
- 自动化集成测试
- 教学和演示

虽然在极限性能上不如C++版本，但在大多数场景下完全满足压测需求，且开发和维护成本更低。