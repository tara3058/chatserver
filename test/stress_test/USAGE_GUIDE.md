# 聊天服务器压测工具完整指南

## 🎯 工具概览

现在你拥有了两套完整的聊天服务器压测工具：

### 1. C++版本 (高性能)
- 位置：`test/stress_test/`
- 特点：基于Muduo网络库，极致性能
- 适用：生产环境大规模压测

### 2. Python版本 (易用性) ⭐ 推荐
- 位置：`test/stress_test/python/`
- 特点：基于asyncio，开发简单，功能完善
- 适用：日常测试、快速验证、学习使用

## 🚀 快速开始 (Python版本)

### 安装依赖
```bash
pip install psutil
```

### 一键压测
```bash
cd test/stress_test/python
python quick_test.py quick 127.0.0.1 6000
```

这将自动执行：
1. ✅ 连接功能测试
2. ✅ 50个客户端压力测试
3. ✅ 性能数据收集

## 📊 压测工具对比

| 工具 | 语言 | 难度 | 性能 | 推荐场景 |
|------|------|------|------|----------|
| **stress_client.py** | Python | ⭐ | ⭐⭐⭐ | 日常测试、快速验证 |
| **stress_client** (C++) | C++ | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 生产环境大规模压测 |
| **tcpkali_test.sh** | Shell | ⭐⭐ | ⭐⭐⭐⭐ | 通用TCP压测 |

## 🔧 详细使用方法

### Python版本 (推荐)

#### 1. 连接测试
```bash
python connection_test.py 127.0.0.1 6000
```
验证：登录、聊天、群聊功能

#### 2. 压力测试
```bash
# 基础压测
python stress_client.py 127.0.0.1 6000 100 60

# 高级压测
python stress_client.py 127.0.0.1 6000 500 300 --interval 0.05
```

#### 3. 性能监控
```bash
python monitor.py --duration 300 --interval 1.0
```
监控：CPU、内存、网络连接、文件描述符

#### 4. 综合压测
```bash
python run_stress_test.py --clients 50,100,200,500 --duration 120
```
自动化：连接测试 → 预热 → 多级压测 → 报告生成

### C++版本 (高性能)

#### 编译
```bash
cd test/stress_test
mkdir build && cd build
cmake .. && make
```

#### 运行
```bash
./stress_client 127.0.0.1 6000 1000 300
./connection_test 127.0.0.1 6000
```

#### 自动化脚本
```bash
./run_stress_test.sh  # 完整压测流程
./monitor.sh          # 性能监控
./tcpkali_test.sh     # tcpkali压测
```

## 📈 压测场景建议

### 开发阶段
```bash
# 功能验证
python quick_test.py test 127.0.0.1 6000

# 基础压测
python stress_client.py 127.0.0.1 6000 50 60
```

### 测试阶段
```bash
# 中等规模
python stress_client.py 127.0.0.1 6000 200 120

# 综合测试
python run_stress_test.py --clients 50,100,200 --duration 120
```

### 生产验证
```bash
# C++高性能压测
./stress_client 127.0.0.1 6000 1000 600

# 或大规模Python压测
python stress_client.py 127.0.0.1 6000 1000 600
```

## 🎯 性能指标参考

### 预期指标
- **并发连接数**: 1000-10000+
- **消息QPS**: 1000-5000+
- **连接成功率**: > 99%
- **登录成功率**: > 95%
- **平均延迟**: < 50ms
- **CPU使用率**: < 80%

### 告警阈值
- 连接成功率 < 95%
- 消息丢失率 > 1%
- 平均延迟 > 100ms
- CPU使用率 > 90%
- 内存增长异常

## 🛠️ 系统优化建议

### 操作系统优化
```bash
# 文件描述符限制
ulimit -n 65536

# 网络参数
echo 65536 > /proc/sys/net/core/somaxconn
echo 65536 > /proc/sys/net/ipv4/tcp_max_syn_backlog
```

### 服务器优化
1. 调整Muduo线程数 (chatserver.cpp 中的 setThreadNum)
2. 优化数据库连接池大小 (mysql.conf)
3. 配置Redis缓存策略
4. 启用编译器优化 (-O2 -DNDEBUG)

### 监控建议
```bash
# 同时运行监控和压测
python monitor.py --duration 300 &
python stress_client.py 127.0.0.1 6000 500 300

# 查看结果
ls performance_monitor_*.csv
ls stress_test_*.log
```

## 🔍 故障排查

### 常见问题
1. **连接拒绝**: 检查服务器启动状态
2. **文件描述符不足**: `ulimit -n 65536`
3. **内存不足**: 减少并发数或增加内存
4. **消息解析错误**: 检查JSON格式

### 调试命令
```bash
# 检查服务器进程
ps aux | grep chatserver

# 检查端口监听
netstat -tuln | grep 6000

# 检查连接数
ss -tan | grep :6000 | wc -l

# 系统资源
htop
iostat 1
```

## 📁 文件结构

```
test/stress_test/
├── python/                    # Python版本 (推荐)
│   ├── stress_client.py      # 主压测客户端
│   ├── connection_test.py    # 连接测试
│   ├── monitor.py           # 性能监控
│   ├── run_stress_test.py   # 综合管理器
│   ├── quick_test.py        # 快速测试
│   └── README.md           # Python版本说明
├── stress_client.cpp          # C++压测客户端
├── connection_test.cpp        # C++连接测试
├── run_stress_test.sh        # 综合压测脚本
├── monitor.sh               # 性能监控脚本
├── tcpkali_test.sh          # tcpkali压测
└── README.md               # 总体说明
```

## 💡 最佳实践

### 1. 压测前检查
- ✅ 服务器正常启动
- ✅ 数据库连接正常
- ✅ Redis服务运行
- ✅ 系统资源充足

### 2. 分阶段压测
- 🔄 功能测试 → 小规模压测 → 逐步增加负载
- 🔄 单机压测 → 分布式压测
- 🔄 短时间测试 → 长时间稳定性测试

### 3. 结果分析
- 📊 对比不同配置下的性能
- 📊 分析瓶颈和优化点
- 📊 制定性能基线和告警策略

## 🎉 总结

现在你拥有了业界级别的聊天服务器压测工具套件：

### Python版本优势
- ✨ **开发简单**: 代码清晰，易于理解和修改
- ✨ **功能完整**: 压测、监控、报告一应俱全
- ✨ **使用便捷**: 一行命令即可开始压测
- ✨ **扩展性强**: 易于添加新功能和自定义

### 建议使用流程
1. **日常开发**: 使用 `python quick_test.py quick`
2. **功能验证**: 使用 `python connection_test.py`
3. **性能测试**: 使用 `python stress_client.py`
4. **生产压测**: 使用 C++ 版本或大规模 Python 压测

开始你的聊天服务器压测之旅吧！ 🚀