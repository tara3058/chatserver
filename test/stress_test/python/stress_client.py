#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
聊天服务器压测客户端 - Python版本
基于asyncio实现高并发TCP连接和消息收发
"""

import asyncio
import json
import time
import argparse
import logging
import signal
import sys
from typing import List, Dict, Optional
from dataclasses import dataclass
from collections import defaultdict
import statistics

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s',
    handlers=[
        logging.StreamHandler(),
        logging.FileHandler(f'stress_test_{int(time.time())}.log')
    ]
)
logger = logging.getLogger(__name__)

@dataclass
class TestResult:
    """测试结果数据类"""
    client_id: int
    connected: bool = False
    messages_sent: int = 0
    messages_received: int = 0
    connect_time: float = 0
    disconnect_time: float = 0
    login_success: bool = False
    errors: List[str] = None
    
    def __post_init__(self):
        if self.errors is None:
            self.errors = []

class ChatClient:
    """聊天客户端类"""
    
    def __init__(self, client_id: int, server_host: str, server_port: int, 
                 test_duration: int, message_interval: float = 0.1):
        self.client_id = client_id
        self.server_host = server_host
        self.server_port = server_port
        self.test_duration = test_duration
        self.message_interval = message_interval
        self.user_id = 1000 + client_id
        
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.result = TestResult(client_id=client_id)
        self.running = False
        self.start_time = 0
        
    async def connect(self) -> bool:
        """连接到服务器"""
        try:
            connect_start = time.time()
            self.reader, self.writer = await asyncio.open_connection(
                self.server_host, self.server_port
            )
            self.result.connect_time = time.time() - connect_start
            self.result.connected = True
            logger.info(f"客户端 {self.client_id} 连接成功")
            return True
        except Exception as e:
            error_msg = f"连接失败: {str(e)}"
            self.result.errors.append(error_msg)
            logger.error(f"客户端 {self.client_id} {error_msg}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self.writer:
            try:
                self.writer.close()
                await self.writer.wait_closed()
                self.result.disconnect_time = time.time()
                logger.info(f"客户端 {self.client_id} 已断开连接")
            except Exception as e:
                logger.error(f"客户端 {self.client_id} 断开连接时出错: {e}")
    
    async def send_message(self, message: Dict) -> bool:
        """发送消息"""
        if not self.writer:
            return False
        
        try:
            msg_str = json.dumps(message, ensure_ascii=False)
            self.writer.write(msg_str.encode('utf-8'))
            await self.writer.drain()
            self.result.messages_sent += 1
            return True
        except Exception as e:
            error_msg = f"发送消息失败: {str(e)}"
            self.result.errors.append(error_msg)
            logger.error(f"客户端 {self.client_id} {error_msg}")
            return False
    
    async def receive_messages(self):
        """接收消息循环"""
        while self.running and self.reader:
            try:
                # 设置超时避免阻塞
                data = await asyncio.wait_for(self.reader.read(4096), timeout=1.0)
                if not data:
                    break
                
                # 解析JSON消息
                try:
                    message_str = data.decode('utf-8')
                    # 处理可能的多个JSON消息
                    for line in message_str.strip().split('\n'):
                        if line.strip():
                            message = json.loads(line.strip())
                            await self.handle_message(message)
                            self.result.messages_received += 1
                except json.JSONDecodeError as e:
                    logger.warning(f"客户端 {self.client_id} JSON解析失败: {e}")
                
            except asyncio.TimeoutError:
                # 超时是正常的，继续循环
                continue
            except Exception as e:
                error_msg = f"接收消息失败: {str(e)}"
                self.result.errors.append(error_msg)
                logger.error(f"客户端 {self.client_id} {error_msg}")
                break
    
    async def handle_message(self, message: Dict):
        """处理接收到的消息"""
        msg_id = message.get('msgid', 0)
        errno = message.get('errno', -1)
        
        if msg_id == 1:  # 登录响应
            if errno == 0:
                self.result.login_success = True
                logger.debug(f"客户端 {self.client_id} 登录成功")
            else:
                error_msg = f"登录失败，错误码: {errno}"
                self.result.errors.append(error_msg)
                logger.warning(f"客户端 {self.client_id} {error_msg}")
    
    async def send_login(self) -> bool:
        """发送登录消息"""
        login_msg = {
            "msgid": 1,
            "id": self.user_id,
            "password": "123456"
        }
        return await self.send_message(login_msg)
    
    async def send_chat_message(self, target_user: int) -> bool:
        """发送聊天消息"""
        chat_msg = {
            "msgid": 5,
            "id": self.user_id,
            "from": self.user_id,
            "to": target_user,
            "msg": f"来自客户端 {self.client_id} 的测试消息 #{self.result.messages_sent}",
            "time": str(int(time.time()))
        }
        return await self.send_message(chat_msg)
    
    async def run_test(self, total_clients: int):
        """运行压测"""
        self.start_time = time.time()
        self.running = True
        
        # 连接服务器
        if not await self.connect():
            return
        
        # 启动消息接收任务
        receive_task = asyncio.create_task(self.receive_messages())
        
        try:
            # 发送登录消息
            await self.send_login()
            await asyncio.sleep(0.5)  # 等待登录响应
            
            # 开始发送聊天消息
            end_time = self.start_time + self.test_duration
            
            while time.time() < end_time and self.running:
                # 随机选择目标用户
                target_user = 1001 + (self.client_id + 1) % total_clients
                await self.send_chat_message(target_user)
                
                # 控制发送频率
                await asyncio.sleep(self.message_interval)
        
        except Exception as e:
            error_msg = f"测试执行异常: {str(e)}"
            self.result.errors.append(error_msg)
            logger.error(f"客户端 {self.client_id} {error_msg}")
        
        finally:
            self.running = False
            receive_task.cancel()
            try:
                await receive_task
            except asyncio.CancelledError:
                pass
            await self.disconnect()

class StressTestManager:
    """压测管理器"""
    
    def __init__(self, server_host: str, server_port: int, 
                 client_count: int, test_duration: int, message_interval: float = 0.1):
        self.server_host = server_host
        self.server_port = server_port
        self.client_count = client_count
        self.test_duration = test_duration
        self.message_interval = message_interval
        self.clients: List[ChatClient] = []
        self.start_time = 0
        self.end_time = 0
        self.running = True
        
    def create_clients(self):
        """创建客户端"""
        self.clients = []
        for i in range(self.client_count):
            client = ChatClient(
                client_id=i,
                server_host=self.server_host,
                server_port=self.server_port,
                test_duration=self.test_duration,
                message_interval=self.message_interval
            )
            self.clients.append(client)
        
        logger.info(f"创建了 {len(self.clients)} 个客户端")
    
    async def run_stress_test(self):
        """运行压力测试"""
        logger.info(f"开始压力测试")
        logger.info(f"服务器: {self.server_host}:{self.server_port}")
        logger.info(f"客户端数量: {self.client_count}")
        logger.info(f"测试时长: {self.test_duration}秒")
        logger.info(f"消息间隔: {self.message_interval}秒")
        
        self.create_clients()
        self.start_time = time.time()
        
        # 创建所有客户端任务
        tasks = []
        for client in self.clients:
            task = asyncio.create_task(client.run_test(self.client_count))
            tasks.append(task)
        
        # 启动进度监控
        monitor_task = asyncio.create_task(self.monitor_progress())
        
        try:
            # 等待所有客户端完成
            await asyncio.gather(*tasks, return_exceptions=True)
        except KeyboardInterrupt:
            logger.info("收到中断信号，停止测试...")
            self.running = False
            for client in self.clients:
                client.running = False
        finally:
            monitor_task.cancel()
            try:
                await monitor_task
            except asyncio.CancelledError:
                pass
        
        self.end_time = time.time()
        self.generate_report()
    
    async def monitor_progress(self):
        """监控测试进度"""
        while self.running:
            await asyncio.sleep(10)  # 每10秒报告一次
            
            connected_count = sum(1 for client in self.clients if client.result.connected)
            total_sent = sum(client.result.messages_sent for client in self.clients)
            total_received = sum(client.result.messages_received for client in self.clients)
            
            elapsed = time.time() - self.start_time
            logger.info(f"进度报告 - 已连接: {connected_count}/{self.client_count}, "
                       f"已发送: {total_sent}, 已接收: {total_received}, "
                       f"用时: {elapsed:.1f}秒")
    
    def generate_report(self):
        """生成测试报告"""
        logger.info("=" * 50)
        logger.info("压力测试报告")
        logger.info("=" * 50)
        
        # 基础统计
        total_duration = self.end_time - self.start_time
        connected_clients = [c for c in self.clients if c.result.connected]
        successful_logins = [c for c in self.clients if c.result.login_success]
        
        logger.info(f"测试配置:")
        logger.info(f"  服务器: {self.server_host}:{self.server_port}")
        logger.info(f"  客户端数量: {self.client_count}")
        logger.info(f"  测试时长: {self.test_duration}秒")
        logger.info(f"  实际运行时长: {total_duration:.2f}秒")
        
        logger.info(f"\n连接统计:")
        logger.info(f"  连接成功率: {len(connected_clients)}/{self.client_count} "
                   f"({len(connected_clients)/self.client_count*100:.1f}%)")
        logger.info(f"  登录成功率: {len(successful_logins)}/{len(connected_clients)} "
                   f"({len(successful_logins)/max(len(connected_clients), 1)*100:.1f}%)")
        
        # 消息统计
        total_sent = sum(c.result.messages_sent for c in self.clients)
        total_received = sum(c.result.messages_received for c in self.clients)
        
        logger.info(f"\n消息统计:")
        logger.info(f"  总发送消息数: {total_sent}")
        logger.info(f"  总接收消息数: {total_received}")
        logger.info(f"  平均QPS: {total_sent/total_duration:.2f}")
        logger.info(f"  每客户端平均发送: {total_sent/self.client_count:.1f}")
        
        # 连接时间统计
        if connected_clients:
            connect_times = [c.result.connect_time for c in connected_clients]
            logger.info(f"\n连接时间统计:")
            logger.info(f"  平均连接时间: {statistics.mean(connect_times):.3f}秒")
            logger.info(f"  最大连接时间: {max(connect_times):.3f}秒")
            logger.info(f"  最小连接时间: {min(connect_times):.3f}秒")
        
        # 错误统计
        total_errors = sum(len(c.result.errors) for c in self.clients)
        if total_errors > 0:
            logger.info(f"\n错误统计:")
            logger.info(f"  总错误数: {total_errors}")
            
            # 按错误类型分组
            error_types = defaultdict(int)
            for client in self.clients:
                for error in client.result.errors:
                    error_type = error.split(':')[0] if ':' in error else error
                    error_types[error_type] += 1
            
            for error_type, count in error_types.items():
                logger.info(f"    {error_type}: {count}次")
        
        logger.info("=" * 50)

def signal_handler(signum, frame):
    """信号处理器"""
    logger.info(f"收到信号 {signum}，准备退出...")
    sys.exit(0)

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='聊天服务器压力测试工具 (Python版)')
    parser.add_argument('host', help='服务器IP地址')
    parser.add_argument('port', type=int, help='服务器端口')
    parser.add_argument('clients', type=int, help='并发客户端数量 (1-10000)')
    parser.add_argument('duration', type=int, help='测试时长(秒) (1-3600)')
    parser.add_argument('--interval', type=float, default=0.1, 
                       help='消息发送间隔(秒) (默认: 0.1)')
    parser.add_argument('--log-level', default='INFO',
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                       help='日志级别 (默认: INFO)')
    
    args = parser.parse_args()
    
    # 参数验证
    if not (1 <= args.clients <= 10000):
        logger.error("客户端数量应在 1-10000 之间")
        return 1
    
    if not (1 <= args.duration <= 3600):
        logger.error("测试时长应在 1-3600 秒之间")
        return 1
    
    # 设置日志级别
    logging.getLogger().setLevel(getattr(logging, args.log_level))
    
    # 注册信号处理器
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    # 创建压测管理器
    manager = StressTestManager(
        server_host=args.host,
        server_port=args.port,
        client_count=args.clients,
        test_duration=args.duration,
        message_interval=args.interval
    )
    
    try:
        await manager.run_stress_test()
        return 0
    except Exception as e:
        logger.error(f"测试执行失败: {e}")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("测试被用户中断")
        sys.exit(0)