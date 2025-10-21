#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
快速压测脚本 - Python版本
提供简单快速的压测功能
"""

import asyncio
import argparse
import logging
import sys
from pathlib import Path

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

async def run_quick_test(host: str, port: int, clients: int, duration: int):
    """运行快速压测"""
    logger.info(f"快速压测 - {clients}个客户端，{duration}秒")
    
    # 导入压测客户端
    sys.path.append(str(Path(__file__).parent))
    from stress_client import StressTestManager
    
    manager = StressTestManager(
        server_host=host,
        server_port=port,
        client_count=clients,
        test_duration=duration,
        message_interval=0.1
    )
    
    try:
        await manager.run_stress_test()
        return True
    except Exception as e:
        logger.error(f"压测失败: {e}")
        return False

async def run_connection_check(host: str, port: int):
    """运行连接检查"""
    logger.info("执行连接检查...")
    
    sys.path.append(str(Path(__file__).parent))
    from connection_test import SimpleTestClient
    
    client = SimpleTestClient(host, port)
    try:
        success = await client.run_all_tests()
        return success
    except Exception as e:
        logger.error(f"连接检查失败: {e}")
        return False

async def run_performance_monitor(duration: int):
    """运行性能监控"""
    logger.info(f"启动性能监控 - {duration}秒")
    
    sys.path.append(str(Path(__file__).parent))
    from monitor import ServerMonitor
    
    monitor = ServerMonitor(
        process_name="chatserver",
        server_port=6000,
        monitor_duration=duration,
        sample_interval=1.0
    )
    
    try:
        success = await monitor.run()
        return success
    except Exception as e:
        logger.error(f"性能监控失败: {e}")
        return False

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='聊天服务器快速压测工具')
    
    subparsers = parser.add_subparsers(dest='command', help='可用命令')
    
    # 压测命令
    stress_parser = subparsers.add_parser('stress', help='运行压力测试')
    stress_parser.add_argument('host', help='服务器IP')
    stress_parser.add_argument('port', type=int, help='服务器端口')
    stress_parser.add_argument('clients', type=int, help='客户端数量')
    stress_parser.add_argument('duration', type=int, help='测试时长(秒)')
    
    # 连接测试命令
    test_parser = subparsers.add_parser('test', help='运行连接测试')
    test_parser.add_argument('host', help='服务器IP')
    test_parser.add_argument('port', type=int, help='服务器端口')
    
    # 监控命令
    monitor_parser = subparsers.add_parser('monitor', help='运行性能监控')
    monitor_parser.add_argument('duration', type=int, help='监控时长(秒)')
    
    # 快速测试命令
    quick_parser = subparsers.add_parser('quick', help='快速测试 (连接测试 + 小规模压测)')
    quick_parser.add_argument('host', default='127.0.0.1', nargs='?', help='服务器IP (默认: 127.0.0.1)')
    quick_parser.add_argument('port', type=int, default=6000, nargs='?', help='服务器端口 (默认: 6000)')
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return 1
    
    success = False  # 初始化success变量
    
    try:
        if args.command == 'stress':
            success = await run_quick_test(args.host, args.port, args.clients, args.duration)
        elif args.command == 'test':
            success = await run_connection_check(args.host, args.port)
        elif args.command == 'monitor':
            success = await run_performance_monitor(args.duration)
        elif args.command == 'quick':
            logger.info("=" * 50)
            logger.info("快速测试模式")
            logger.info("=" * 50)
            
            # 1. 连接测试
            logger.info("1. 连接功能测试")
            test_success = await run_connection_check(args.host, args.port)
            
            if test_success:
                logger.info("连接测试通过，开始压力测试...")
                await asyncio.sleep(2)
                
                # 2. 小规模压测
                logger.info("2. 小规模压力测试 (50客户端 × 60秒)")
                stress_success = await run_quick_test(args.host, args.port, 50, 60)
                success = stress_success
            else:
                logger.error("连接测试失败，跳过压力测试")
                success = False
        
        return 0 if success else 1
        
    except KeyboardInterrupt:
        logger.info("测试被用户中断")
        return 1
    except Exception as e:
        logger.error(f"执行失败: {e}")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("程序被用户中断")
        sys.exit(1)