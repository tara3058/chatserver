#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
服务器性能监控工具 - Python版本
实时监控CPU、内存、网络连接等指标
"""

import asyncio
import psutil
import time
import argparse
import logging
import json
import csv
import signal
import sys
from typing import Dict, List, Optional
from dataclasses import dataclass, asdict
from datetime import datetime

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

@dataclass
class PerformanceMetrics:
    """性能指标数据类"""
    timestamp: str
    cpu_percent: float
    memory_rss_mb: float
    memory_vms_mb: float
    memory_percent: float
    open_files: int
    connections_total: int
    connections_established: int
    network_bytes_sent: int
    network_bytes_recv: int

class ServerMonitor:
    """服务器性能监控器"""
    
    def __init__(self, process_name: str = "chatserver", server_port: int = 6000, 
                 monitor_duration: int = 300, sample_interval: float = 1.0):
        self.process_name = process_name
        self.server_port = server_port
        self.monitor_duration = monitor_duration
        self.sample_interval = sample_interval
        self.process: Optional[psutil.Process] = None
        self.metrics_history: List[PerformanceMetrics] = []
        self.running = True
        self.start_time = time.time()
        
    def find_server_process(self) -> bool:
        """查找服务器进程"""
        try:
            # 通过进程名查找
            for proc in psutil.process_iter(['pid', 'name', 'cmdline']):
                try:
                    proc_info = proc.info
                    if (self.process_name.lower() in proc_info['name'].lower() or
                        any(self.process_name.lower() in cmd.lower() 
                            for cmd in proc_info.get('cmdline', []))):
                        
                        self.process = psutil.Process(proc.pid)
                        logger.info(f"找到服务器进程: PID={proc.pid}, 名称={proc_info['name']}")
                        return True
                except (psutil.NoSuchProcess, psutil.AccessDenied):
                    continue
            
            logger.error(f"未找到服务器进程: {self.process_name}")
            return False
            
        except Exception as e:
            logger.error(f"查找进程时出错: {e}")
            return False
    
    def get_network_connections(self) -> tuple:
        """获取网络连接统计"""
        try:
            connections = psutil.net_connections()
            
            total_connections = 0
            established_connections = 0
            
            for conn in connections:
                if conn.laddr and conn.laddr.port == self.server_port:
                    total_connections += 1
                    if conn.status == psutil.CONN_ESTABLISHED:
                        established_connections += 1
            
            return total_connections, established_connections
            
        except Exception as e:
            logger.warning(f"获取网络连接失败: {e}")
            return 0, 0
    
    def collect_metrics(self) -> Optional[PerformanceMetrics]:
        """收集性能指标"""
        if not self.process:
            return None
        
        try:
            # 检查进程是否还在运行
            if not self.process.is_running():
                logger.warning("服务器进程已停止")
                return None
            
            # CPU使用率
            cpu_percent = self.process.cpu_percent()
            
            # 内存使用情况
            memory_info = self.process.memory_info()
            memory_percent = self.process.memory_percent()
            
            # 打开的文件数
            try:
                open_files = self.process.num_fds() if hasattr(self.process, 'num_fds') else len(self.process.open_files())
            except (psutil.AccessDenied, AttributeError):
                open_files = 0
            
            # 网络连接
            total_conn, established_conn = self.get_network_connections()
            
            # 网络IO统计
            try:
                net_io = psutil.net_io_counters()
                bytes_sent = net_io.bytes_sent if net_io else 0
                bytes_recv = net_io.bytes_recv if net_io else 0
            except Exception:
                bytes_sent = bytes_recv = 0
            
            metrics = PerformanceMetrics(
                timestamp=datetime.now().strftime('%Y-%m-%d %H:%M:%S'),
                cpu_percent=cpu_percent,
                memory_rss_mb=memory_info.rss / 1024 / 1024,
                memory_vms_mb=memory_info.vms / 1024 / 1024,
                memory_percent=memory_percent,
                open_files=open_files,
                connections_total=total_conn,
                connections_established=established_conn,
                network_bytes_sent=bytes_sent,
                network_bytes_recv=bytes_recv
            )
            
            return metrics
            
        except (psutil.NoSuchProcess, psutil.AccessDenied) as e:
            logger.error(f"获取进程信息失败: {e}")
            return None
        except Exception as e:
            logger.error(f"收集指标时出错: {e}")
            return None
    
    async def monitor_loop(self):
        """监控循环"""
        logger.info("开始性能监控...")
        logger.info(f"监控时长: {self.monitor_duration}秒")
        logger.info(f"采样间隔: {self.sample_interval}秒")
        
        sample_count = 0
        end_time = self.start_time + self.monitor_duration
        
        while self.running and time.time() < end_time:
            metrics = self.collect_metrics()
            
            if metrics:
                self.metrics_history.append(metrics)
                sample_count += 1
                
                # 每10个样本报告一次
                if sample_count % 10 == 0:
                    logger.info(f"[{metrics.timestamp}] CPU: {metrics.cpu_percent:.1f}% | "
                               f"内存: {metrics.memory_rss_mb:.1f}MB | "
                               f"连接数: {metrics.connections_established}")
            
            await asyncio.sleep(self.sample_interval)
        
        logger.info(f"监控结束，共收集 {len(self.metrics_history)} 个样本")
    
    def save_metrics_to_csv(self, filename: str):
        """保存指标到CSV文件"""
        try:
            with open(filename, 'w', newline='', encoding='utf-8') as csvfile:
                if self.metrics_history:
                    fieldnames = asdict(self.metrics_history[0]).keys()
                    writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
                    
                    writer.writeheader()
                    for metrics in self.metrics_history:
                        writer.writerow(asdict(metrics))
            
            logger.info(f"性能数据已保存到: {filename}")
            
        except Exception as e:
            logger.error(f"保存CSV文件失败: {e}")
    
    def save_metrics_to_json(self, filename: str):
        """保存指标到JSON文件"""
        try:
            data = {
                'monitor_info': {
                    'process_name': self.process_name,
                    'server_port': self.server_port,
                    'monitor_duration': self.monitor_duration,
                    'sample_interval': self.sample_interval,
                    'start_time': datetime.fromtimestamp(self.start_time).isoformat(),
                    'sample_count': len(self.metrics_history)
                },
                'metrics': [asdict(m) for m in self.metrics_history]
            }
            
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2, ensure_ascii=False)
            
            logger.info(f"性能数据已保存到: {filename}")
            
        except Exception as e:
            logger.error(f"保存JSON文件失败: {e}")
    
    def generate_report(self):
        """生成性能报告"""
        if not self.metrics_history:
            logger.warning("没有收集到性能数据")
            return
        
        logger.info("=" * 60)
        logger.info("性能监控报告")
        logger.info("=" * 60)
        
        # 基本信息
        logger.info(f"监控对象: {self.process_name} (PID: {self.process.pid if self.process else 'N/A'})")
        logger.info(f"监控端口: {self.server_port}")
        logger.info(f"监控时长: {self.monitor_duration}秒")
        logger.info(f"样本数量: {len(self.metrics_history)}")
        
        # CPU统计
        cpu_values = [m.cpu_percent for m in self.metrics_history]
        logger.info(f"\nCPU使用率:")
        logger.info(f"  平均值: {sum(cpu_values)/len(cpu_values):.2f}%")
        logger.info(f"  最大值: {max(cpu_values):.2f}%")
        logger.info(f"  最小值: {min(cpu_values):.2f}%")
        
        # 内存统计
        memory_values = [m.memory_rss_mb for m in self.metrics_history]
        logger.info(f"\n内存使用量 (RSS):")
        logger.info(f"  平均值: {sum(memory_values)/len(memory_values):.2f}MB")
        logger.info(f"  最大值: {max(memory_values):.2f}MB")
        logger.info(f"  最小值: {min(memory_values):.2f}MB")
        
        # 连接数统计
        conn_values = [m.connections_established for m in self.metrics_history]
        logger.info(f"\nTCP连接数 (ESTABLISHED):")
        logger.info(f"  平均值: {sum(conn_values)/len(conn_values):.1f}")
        logger.info(f"  最大值: {max(conn_values)}")
        logger.info(f"  最小值: {min(conn_values)}")
        
        # 文件描述符统计
        fd_values = [m.open_files for m in self.metrics_history]
        logger.info(f"\n文件描述符数:")
        logger.info(f"  平均值: {sum(fd_values)/len(fd_values):.1f}")
        logger.info(f"  最大值: {max(fd_values)}")
        logger.info(f"  最小值: {min(fd_values)}")
        
        # 网络统计
        if len(self.metrics_history) > 1:
            bytes_sent_start = self.metrics_history[0].network_bytes_sent
            bytes_sent_end = self.metrics_history[-1].network_bytes_sent
            bytes_recv_start = self.metrics_history[0].network_bytes_recv
            bytes_recv_end = self.metrics_history[-1].network_bytes_recv
            
            duration = len(self.metrics_history) * self.sample_interval
            sent_rate = (bytes_sent_end - bytes_sent_start) / duration / 1024  # KB/s
            recv_rate = (bytes_recv_end - bytes_recv_start) / duration / 1024  # KB/s
            
            logger.info(f"\n网络吞吐量:")
            logger.info(f"  发送速率: {sent_rate:.2f} KB/s")
            logger.info(f"  接收速率: {recv_rate:.2f} KB/s")
        
        logger.info("=" * 60)
    
    def check_system_limits(self):
        """检查系统限制"""
        logger.info("\n系统限制检查:")
        
        try:
            # 文件描述符限制
            import resource
            soft_limit, hard_limit = resource.getrlimit(resource.RLIMIT_NOFILE)
            logger.info(f"  文件描述符限制: {soft_limit} (软限制) / {hard_limit} (硬限制)")
        except Exception:
            logger.info("  无法获取文件描述符限制")
        
        try:
            # 系统负载
            load_avg = psutil.getloadavg()
            logger.info(f"  系统负载: {load_avg[0]:.2f} (1分钟) / {load_avg[1]:.2f} (5分钟) / {load_avg[2]:.2f} (15分钟)")
        except Exception:
            logger.info("  无法获取系统负载")
        
        try:
            # 内存使用情况
            memory = psutil.virtual_memory()
            logger.info(f"  系统内存: 总计 {memory.total/1024/1024/1024:.1f}GB, "
                       f"已用 {memory.used/1024/1024/1024:.1f}GB ({memory.percent:.1f}%)")
        except Exception:
            logger.info("  无法获取系统内存信息")
    
    async def run(self):
        """运行监控"""
        if not self.find_server_process():
            return False
        
        # 显示系统限制
        self.check_system_limits()
        
        try:
            await self.monitor_loop()
            
            # 生成报告
            self.generate_report()
            
            # 保存数据
            timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
            csv_file = f"performance_monitor_{timestamp}.csv"
            json_file = f"performance_monitor_{timestamp}.json"
            
            self.save_metrics_to_csv(csv_file)
            self.save_metrics_to_json(json_file)
            
            return True
            
        except Exception as e:
            logger.error(f"监控过程中出错: {e}")
            return False

def signal_handler(signum, frame, monitor):
    """信号处理器"""
    logger.info(f"收到信号 {signum}，停止监控...")
    monitor.running = False

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='服务器性能监控工具')
    parser.add_argument('--process', default='chatserver', help='服务器进程名 (默认: chatserver)')
    parser.add_argument('--port', type=int, default=6000, help='服务器端口 (默认: 6000)')
    parser.add_argument('--duration', type=int, default=300, help='监控时长(秒) (默认: 300)')
    parser.add_argument('--interval', type=float, default=1.0, help='采样间隔(秒) (默认: 1.0)')
    parser.add_argument('--log-level', default='INFO',
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                       help='日志级别 (默认: INFO)')
    
    args = parser.parse_args()
    
    # 设置日志级别
    logging.getLogger().setLevel(getattr(logging, args.log_level))
    
    # 创建监控器
    monitor = ServerMonitor(
        process_name=args.process,
        server_port=args.port,
        monitor_duration=args.duration,
        sample_interval=args.interval
    )
    
    # 注册信号处理器
    signal.signal(signal.SIGINT, lambda s, f: signal_handler(s, f, monitor))
    signal.signal(signal.SIGTERM, lambda s, f: signal_handler(s, f, monitor))
    
    try:
        success = await monitor.run()
        return 0 if success else 1
    except KeyboardInterrupt:
        logger.info("监控被用户中断")
        monitor.running = False
        if monitor.metrics_history:
            monitor.generate_report()
        return 0
    except Exception as e:
        logger.error(f"监控执行失败: {e}")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("监控被用户中断")
        sys.exit(0)