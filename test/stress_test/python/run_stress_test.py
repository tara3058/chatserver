#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
聊天服务器综合压测管理器 - Python版本
自动化执行完整的压测流程
"""

import asyncio
import subprocess
import time
import argparse
import logging
import sys
import json
import socket
from pathlib import Path
from typing import List, Dict, Optional
from datetime import datetime

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class StressTestManager:
    """压测管理器"""
    
    def __init__(self, server_host: str, server_port: int, 
                 python_dir: str = "./python"):
        self.server_host = server_host
        self.server_port = server_port
        self.python_dir = Path(python_dir)
        self.results_dir = Path("results")
        
        # 压测配置
        self.warmup_config = {"clients": 10, "duration": 30}
        self.stress_levels = [50, 100, 200, 500]
        self.stress_duration = 120
        
        # 创建结果目录
        self.results_dir.mkdir(exist_ok=True)
        
        # 时间戳
        self.timestamp = datetime.now().strftime('%Y%m%d_%H%M%S')
    
    def check_server_connection(self) -> bool:
        """检查服务器连接"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            result = sock.connect_ex((self.server_host, self.server_port))
            sock.close()
            return result == 0
        except Exception as e:
            logger.error(f"检查服务器连接失败: {e}")
            return False
    
    def check_dependencies(self) -> bool:
        """检查依赖"""
        logger.info("检查依赖...")
        
        # 检查Python脚本文件
        required_scripts = [
            "stress_client.py",
            "connection_test.py", 
            "monitor.py"
        ]
        
        for script in required_scripts:
            script_path = self.python_dir / script
            if not script_path.exists():
                logger.error(f"缺少脚本文件: {script_path}")
                return False
        
        # 检查Python依赖
        try:
            import psutil
            import asyncio
        except ImportError as e:
            logger.error(f"缺少Python依赖: {e}")
            logger.info("请安装: pip install psutil")
            return False
        
        # 检查服务器连接
        if not self.check_server_connection():
            logger.error(f"无法连接到服务器 {self.server_host}:{self.server_port}")
            logger.info("请确保聊天服务器正在运行")
            return False
        
        logger.info("依赖检查通过")
        return True
    
    async def run_script(self, script_name: str, args: List[str], 
                        timeout: Optional[int] = None) -> Dict:
        """运行Python脚本"""
        script_path = self.python_dir / script_name
        cmd = [sys.executable, str(script_path)] + args
        
        logger.info(f"执行命令: {' '.join(cmd)}")
        
        try:
            process = await asyncio.create_subprocess_exec(
                *cmd,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            
            try:
                stdout, stderr = await asyncio.wait_for(
                    process.communicate(), timeout=timeout
                )
                
                return {
                    "returncode": process.returncode,
                    "stdout": stdout.decode('utf-8'),
                    "stderr": stderr.decode('utf-8'),
                    "success": process.returncode == 0
                }
            except asyncio.TimeoutError:
                process.kill()
                await process.wait()
                return {
                    "returncode": -1,
                    "stdout": "",
                    "stderr": "执行超时",
                    "success": False
                }
        
        except Exception as e:
            return {
                "returncode": -1,
                "stdout": "",
                "stderr": str(e),
                "success": False
            }
    
    async def run_connection_test(self) -> bool:
        """运行连接测试"""
        logger.info("=" * 50)
        logger.info("执行连接测试")
        logger.info("=" * 50)
        
        args = [self.server_host, str(self.server_port)]
        result = await self.run_script("connection_test.py", args, timeout=60)
        
        if result["success"]:
            logger.info("✓ 连接测试通过")
            return True
        else:
            logger.error("✗ 连接测试失败")
            logger.error(f"错误信息: {result['stderr']}")
            return False
    
    async def run_warmup_test(self) -> bool:
        """运行预热测试"""
        logger.info("=" * 50)
        logger.info("执行预热测试")
        logger.info("=" * 50)
        
        args = [
            self.server_host,
            str(self.server_port),
            str(self.warmup_config["clients"]),
            str(self.warmup_config["duration"]),
            "--log-level", "WARNING"  # 减少日志输出
        ]
        
        result = await self.run_script("stress_client.py", args, 
                                     timeout=self.warmup_config["duration"] + 30)
        
        if result["success"]:
            logger.info("✓ 预热测试完成")
            return True
        else:
            logger.error("✗ 预热测试失败")
            logger.error(f"错误信息: {result['stderr']}")
            return False
    
    async def run_stress_test_level(self, client_count: int) -> Dict:
        """运行单个压力测试级别"""
        logger.info(f"开始压力测试: {client_count} 个客户端")
        
        # 启动性能监控
        monitor_task = None
        try:
            monitor_args = [
                "--duration", str(self.stress_duration + 10),
                "--interval", "1.0",
                "--log-level", "WARNING"
            ]
            
            # 异步启动监控
            logger.info("启动性能监控...")
            monitor_process = await asyncio.create_subprocess_exec(
                sys.executable, str(self.python_dir / "monitor.py"), *monitor_args,
                stdout=asyncio.subprocess.PIPE,
                stderr=asyncio.subprocess.PIPE
            )
            
            # 等待监控启动
            await asyncio.sleep(2)
            
            # 执行压力测试
            stress_args = [
                self.server_host,
                str(self.server_port),
                str(client_count),
                str(self.stress_duration),
                "--log-level", "INFO"
            ]
            
            stress_result = await self.run_script(
                "stress_client.py", stress_args,
                timeout=self.stress_duration + 60
            )
            
            # 等待监控完成
            try:
                monitor_stdout, monitor_stderr = await asyncio.wait_for(
                    monitor_process.communicate(), timeout=30
                )
                monitor_success = monitor_process.returncode == 0
            except asyncio.TimeoutError:
                monitor_process.kill()
                await monitor_process.wait()
                monitor_success = False
                monitor_stdout = monitor_stderr = b""
            
            return {
                "client_count": client_count,
                "stress_result": stress_result,
                "monitor_result": {
                    "success": monitor_success,
                    "stdout": monitor_stdout.decode('utf-8'),
                    "stderr": monitor_stderr.decode('utf-8')
                }
            }
        
        except Exception as e:
            logger.error(f"压力测试执行异常: {e}")
            return {
                "client_count": client_count,
                "stress_result": {"success": False, "stderr": str(e)},
                "monitor_result": {"success": False, "stderr": str(e)}
            }
    
    async def run_all_stress_tests(self) -> List[Dict]:
        """运行所有压力测试"""
        logger.info("=" * 50)
        logger.info("开始多级压力测试")
        logger.info("=" * 50)
        
        results = []
        
        for client_count in self.stress_levels:
            logger.info(f"\n{'='*20} {client_count} 客户端测试 {'='*20}")
            
            result = await self.run_stress_test_level(client_count)
            results.append(result)
            
            if result["stress_result"]["success"]:
                logger.info(f"✓ {client_count} 客户端测试完成")
            else:
                logger.error(f"✗ {client_count} 客户端测试失败")
                logger.error(f"错误: {result['stress_result']['stderr']}")
            
            # 测试间隔
            if client_count != self.stress_levels[-1]:
                logger.info("等待10秒后继续下一级测试...")
                await asyncio.sleep(10)
        
        return results
    
    def generate_summary_report(self, test_results: List[Dict]):
        """生成汇总报告"""
        logger.info("=" * 60)
        logger.info("压力测试汇总报告")
        logger.info("=" * 60)
        
        # 基本信息
        logger.info(f"测试时间: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
        logger.info(f"服务器: {self.server_host}:{self.server_port}")
        logger.info(f"预热配置: {self.warmup_config['clients']}客户端 × {self.warmup_config['duration']}秒")
        logger.info(f"压测级别: {self.stress_levels}")
        logger.info(f"每级时长: {self.stress_duration}秒")
        
        # 测试结果统计
        logger.info(f"\n测试结果汇总:")
        successful_tests = 0
        
        for result in test_results:
            client_count = result["client_count"]
            success = result["stress_result"]["success"]
            status = "✓ 成功" if success else "✗ 失败"
            
            logger.info(f"  {client_count:3d} 客户端: {status}")
            if success:
                successful_tests += 1
        
        logger.info(f"\n总体统计:")
        logger.info(f"  成功测试: {successful_tests}/{len(test_results)}")
        logger.info(f"  成功率: {successful_tests/len(test_results)*100:.1f}%")
        
        # 保存详细结果
        self.save_test_results(test_results)
        
        logger.info("=" * 60)
    
    def save_test_results(self, test_results: List[Dict]):
        """保存测试结果"""
        try:
            result_file = self.results_dir / f"stress_test_results_{self.timestamp}.json"
            
            summary_data = {
                "test_info": {
                    "timestamp": self.timestamp,
                    "server": f"{self.server_host}:{self.server_port}",
                    "warmup_config": self.warmup_config,
                    "stress_levels": self.stress_levels,
                    "stress_duration": self.stress_duration
                },
                "results": test_results
            }
            
            with open(result_file, 'w', encoding='utf-8') as f:
                json.dump(summary_data, f, indent=2, ensure_ascii=False)
            
            logger.info(f"测试结果已保存到: {result_file}")
            
        except Exception as e:
            logger.error(f"保存测试结果失败: {e}")
    
    async def run_complete_test(self) -> bool:
        """运行完整的压测流程"""
        logger.info("🚀 开始聊天服务器综合压测")
        logger.info(f"时间戳: {self.timestamp}")
        
        try:
            # 1. 检查依赖
            if not self.check_dependencies():
                return False
            
            # 2. 连接测试
            if not await self.run_connection_test():
                return False
            
            # 3. 预热测试
            if not await self.run_warmup_test():
                logger.warning("预热测试失败，但继续执行压力测试...")
            
            # 等待一下再开始压力测试
            logger.info("等待5秒后开始压力测试...")
            await asyncio.sleep(5)
            
            # 4. 多级压力测试
            test_results = await self.run_all_stress_tests()
            
            # 5. 生成报告
            self.generate_summary_report(test_results)
            
            logger.info("🎉 压测完成！")
            return True
            
        except KeyboardInterrupt:
            logger.info("测试被用户中断")
            return False
        except Exception as e:
            logger.error(f"测试执行失败: {e}")
            return False

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='聊天服务器综合压测管理器')
    parser.add_argument('--server', default='127.0.0.1', help='服务器IP (默认: 127.0.0.1)')
    parser.add_argument('--port', type=int, default=6000, help='服务器端口 (默认: 6000)')
    parser.add_argument('--clients', help='压测客户端级别，逗号分隔 (默认: 50,100,200,500)')
    parser.add_argument('--duration', type=int, default=120, help='每级测试时长(秒) (默认: 120)')
    parser.add_argument('--warmup-clients', type=int, default=10, help='预热客户端数 (默认: 10)')
    parser.add_argument('--warmup-duration', type=int, default=30, help='预热时长(秒) (默认: 30)')
    parser.add_argument('--python-dir', default='./python', help='Python脚本目录 (默认: ./python)')
    parser.add_argument('--log-level', default='INFO',
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                       help='日志级别 (默认: INFO)')
    
    args = parser.parse_args()
    
    # 设置日志级别
    logging.getLogger().setLevel(getattr(logging, args.log_level))
    
    # 创建管理器
    manager = StressTestManager(
        server_host=args.server,
        server_port=args.port,
        python_dir=args.python_dir
    )
    
    # 配置参数
    if args.clients:
        try:
            manager.stress_levels = [int(x.strip()) for x in args.clients.split(',')]
        except ValueError:
            logger.error("客户端级别格式错误，使用默认值")
    
    manager.stress_duration = args.duration
    manager.warmup_config = {
        "clients": args.warmup_clients,
        "duration": args.warmup_duration
    }
    
    # 显示配置
    logger.info("压测配置:")
    logger.info(f"  服务器: {args.server}:{args.port}")
    logger.info(f"  预热: {manager.warmup_config['clients']}客户端 × {manager.warmup_config['duration']}秒")
    logger.info(f"  压测级别: {manager.stress_levels}")
    logger.info(f"  每级时长: {manager.stress_duration}秒")
    
    # 确认开始
    try:
        input("\n按 Enter 开始压测，或 Ctrl+C 取消...")
    except KeyboardInterrupt:
        logger.info("压测已取消")
        return 1
    
    # 运行测试
    try:
        success = await manager.run_complete_test()
        return 0 if success else 1
    except KeyboardInterrupt:
        logger.info("压测被用户中断")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("程序被用户中断")
        sys.exit(1)