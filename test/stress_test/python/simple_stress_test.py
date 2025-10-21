#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
简化版聊天服务器压测工具
无需额外依赖，只使用Python标准库
"""

import asyncio
import json
import time
import sys
import socket
import argparse
import logging
from typing import List, Optional

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SimpleChatClient:
    """简化版聊天客户端"""
    
    def __init__(self, client_id: int, host: str, port: int, user_pool: Optional[List[dict]] = None):
        self.client_id = client_id
        self.host = host
        self.port = port
        self.user_id = 1000 + client_id
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.connected = False
        self.messages_sent = 0
        self.messages_received = 0
        self.user_pool = user_pool or []  # 可用的用户池
        self.registered_user_info = None  # 当前用户的注册信息
        
    async def connect(self) -> bool:
        """连接到服务器"""
        try:
            self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
            self.connected = True
            logger.info(f"客户端 {self.client_id} 连接成功")
            return True
        except Exception as e:
            logger.error(f"客户端 {self.client_id} 连接失败: {e}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self.writer:
            try:
                self.writer.close()
                # 更温和地等待连接关闭，并忽略常见的断连错误
                try:
                    await asyncio.wait_for(self.writer.wait_closed(), timeout=1.0)
                except (ConnectionResetError, BrokenPipeError, OSError):
                    # 这些错误在连接关闭时是正常的，不需要记录
                    pass
                self.connected = False
                logger.debug(f"客户端 {self.client_id} 已断开连接")
            except Exception as e:
                # 只记录真正意外的错误
                if "Connection reset by peer" not in str(e):
                    logger.error(f"断开连接时出错: {e}")
    
    async def send_message(self, message: dict) -> bool:
        """发送消息"""
        if not self.writer or not self.connected:
            return False
        
        try:
            msg_str = json.dumps(message, ensure_ascii=False) + "\n"
            self.writer.write(msg_str.encode('utf-8'))
            await self.writer.drain()
            self.messages_sent += 1
            return True
        except (ConnectionResetError, BrokenPipeError, OSError) as e:
            # 连接问题，标记为断开
            self.connected = False
            return False
        except Exception as e:
            logger.error(f"客户端 {self.client_id} 发送消息失败: {e}")
            return False
    
    async def receive_messages(self, duration: int):
        """接收消息"""
        if not self.reader:
            return
        
        end_time = time.time() + duration
        while time.time() < end_time and self.connected:
            try:
                data = await asyncio.wait_for(self.reader.read(4096), timeout=1.0)
                if not data:
                    break
                
                # 简单计数，不解析JSON以避免解析错误
                message_str = data.decode('utf-8', errors='ignore')
                # 按换行符分割消息
                messages = message_str.strip().split('\n')
                self.messages_received += len([m for m in messages if m.strip()])
                
            except asyncio.TimeoutError:
                continue
            except (ConnectionResetError, BrokenPipeError, OSError):
                # 连接被重置，正常结束
                break
            except Exception as e:
                logger.warning(f"客户端 {self.client_id} 接收消息异常: {e}")
                break
    
    async def send_login(self) -> bool:
        """发送登录消息"""
        login_msg = {
            "msgid": 1,
            "id": self.user_id,
            "password": "123456"
        }
        return await self.send_message(login_msg)
    
    async def send_chat_message(self, target_user: Optional[int] = None) -> bool:
        """发送聊天消息"""
        # 如果没有指定目标用户，从用户池中随机选择
        if target_user is None and self.user_pool:
            import random
            # 排除自己，随机选择其他用户
            available_users = [u for u in self.user_pool if u['id'] != self.user_id]
            if available_users:
                target_user = random.choice(available_users)['id']
            else:
                target_user = self.user_id  # 如果没有其他用户，发给自己
        elif target_user is None:
            # 如果没有用户池，使用默认逻辑
            target_user = 1001 + (self.client_id + 1) % 10
        
        chat_msg = {
            "msgid": 5,
            "id": self.user_id,
            "from": self.user_id,
            "to": target_user,
            "msg": f"测试消息来自客户端{self.client_id}",
            "time": str(int(time.time()))
        }
        return await self.send_message(chat_msg)
    
    async def register_user(self, username: str, password: str) -> bool:
        """注册用户"""
        reg_msg = {
            "msgid": 3,  # REG_MSG
            "name": username,
            "password": password
        }
        
        if not await self.send_message(reg_msg):
            return False
        
        # 等待注册响应
        if not self.reader:
            return False
            
        end_time = time.time() + 5  # 5秒超时
        while time.time() < end_time:
            try:
                data = await asyncio.wait_for(self.reader.read(4096), timeout=1.0)
                if data:
                    message_str = data.decode('utf-8', errors='ignore').strip()
                    for line in message_str.split('\n'):
                        if line.strip():
                            try:
                                response = json.loads(line)
                                if response.get('msgid') == 4:  # REG_MSG_ACK
                                    if response.get('errno') == 0:
                                        self.user_id = response.get('id', self.user_id)
                                        self.registered_user_info = {
                                            'id': self.user_id,
                                            'username': username,
                                            'password': password
                                        }
                                        logger.info(f"客户端 {self.client_id} 注册成功: {username} (ID: {self.user_id})")
                                        return True
                                    else:
                                        logger.error(f"客户端 {self.client_id} 注册失败: {username}")
                                        return False
                            except json.JSONDecodeError:
                                continue
            except asyncio.TimeoutError:
                continue
            except Exception:
                break
        
        logger.error(f"客户端 {self.client_id} 注册超时: {username}")
        return False

class SimpleStressTest:
    """简化版压力测试"""
    
    def __init__(self, host: str, port: int, client_count: int, duration: int, use_registration: bool = False):
        self.host = host
        self.port = port
        self.client_count = client_count
        self.duration = duration
        self.use_registration = use_registration
        self.clients: List[SimpleChatClient] = []
        self.user_pool: List[dict] = []  # 存储已注册用户信息
        self.successful_registrations = 0
    
    def check_server(self) -> bool:
        """检查服务器是否可连接"""
        try:
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            sock.settimeout(5)
            result = sock.connect_ex((self.host, self.port))
            sock.close()
            return result == 0
        except Exception:
            return False
    
    async def run_test(self):
        """运行压力测试"""
        logger.info(f"开始压力测试")
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"客户端数量: {self.client_count}")
        logger.info(f"测试时长: {self.duration}秒")
        
        if not self.check_server():
            logger.error(f"无法连接到服务器 {self.host}:{self.port}")
            return False
        
    async def register_users_phase(self) -> bool:
        """注册用户阶段"""
        if not self.use_registration:
            return True
            
        logger.info("开始批量注册用户...")
        
        # 创建注册任务
        register_tasks = []
        for i, client in enumerate(self.clients):
            if client.connected:
                username = f"testuser_{i:04d}"
                password = f"pass_{i:04d}"
                register_tasks.append(client.register_user(username, password))
        
        # 执行注册
        results = await asyncio.gather(*register_tasks, return_exceptions=True)
        
        # 统计注册结果
        self.successful_registrations = sum(1 for result in results if result is True)
        
        # 收集注册成功的用户信息
        for client in self.clients:
            if client.registered_user_info:
                self.user_pool.append(client.registered_user_info)
        
        logger.info(f"注册完成: {self.successful_registrations}/{len(register_tasks)} 成功")
        logger.info(f"用户池大小: {len(self.user_pool)}")
        
        # 更新所有客户端的用户池
        for client in self.clients:
            client.user_pool = self.user_pool
        
        return self.successful_registrations > 0
        
        start_time = time.time()
        
        # 连接所有客户端
        logger.info("连接客户端...")
        connect_tasks = []
        for client in self.clients:
            connect_tasks.append(client.connect())
        
        connect_results = await asyncio.gather(*connect_tasks, return_exceptions=True)
        connected_count = sum(1 for result in connect_results if result is True)
        
        logger.info(f"成功连接 {connected_count}/{self.client_count} 个客户端")
        
        if connected_count == 0:
            logger.error("没有客户端连接成功")
            return False
        
        # 发送登录消息
        logger.info("发送登录消息...")
        login_tasks = []
        for client in self.clients:
            if client.connected:
                login_tasks.append(client.send_login())
        
        await asyncio.gather(*login_tasks, return_exceptions=True)
        
        # 等待登录处理
        await asyncio.sleep(1)
        
        # 开始压测
        logger.info("开始发送测试消息...")
        
        # 启动消息接收任务
        receive_tasks = []
        for client in self.clients:
            if client.connected:
                receive_tasks.append(client.receive_messages(self.duration))
        
        # 启动消息发送任务
        async def send_messages():
            end_time = time.time() + self.duration
            message_count = 0
            
            while time.time() < end_time:
                # 让每个连接的客户端发送一条消息
                send_tasks = []
                for i, client in enumerate(self.clients):
                    if client.connected:
                        target_user = 1001 + (i + 1) % len(self.clients)
                        send_tasks.append(client.send_chat_message(target_user))
                
                await asyncio.gather(*send_tasks, return_exceptions=True)
                message_count += len(send_tasks)
                
                # 控制发送频率
                await asyncio.sleep(0.5)
        
        # 同时运行发送和接收任务
        try:
            await asyncio.gather(send_messages(), *receive_tasks)
        except Exception as e:
            logger.error(f"测试过程中出现异常: {e}")
        
        # 计算结果
        end_time = time.time()
        actual_duration = end_time - start_time
        
        total_sent = sum(client.messages_sent for client in self.clients)
        total_received = sum(client.messages_received for client in self.clients)
        connected_clients = sum(1 for client in self.clients if client.connected)
        
        # 断开所有连接
        logger.info("断开客户端连接...")
        disconnect_tasks = []
        for client in self.clients:
            disconnect_tasks.append(client.disconnect())
        
        await asyncio.gather(*disconnect_tasks, return_exceptions=True)
        
        # 生成报告
        self.generate_report(actual_duration, connected_clients, total_sent, total_received)
        
        return True
    
    def generate_report(self, duration: float, connected: int, sent: int, received: int):
        """生成测试报告"""
        logger.info("=" * 50)
        logger.info("压力测试报告")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"测试时长: {duration:.2f}秒")
        logger.info(f"目标客户端数: {self.client_count}")
        logger.info(f"成功连接数: {connected}")
        logger.info(f"连接成功率: {connected/self.client_count*100:.1f}%")
        logger.info(f"总发送消息数: {sent}")
        logger.info(f"总接收消息数: {received}")
        logger.info(f"平均QPS: {sent/duration:.2f}")
        logger.info(f"每客户端平均发送: {sent/max(connected, 1):.1f}")
        logger.info("=" * 50)

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='简化版聊天服务器压力测试')
    parser.add_argument('host', nargs='?', default='127.0.0.1', help='服务器IP (默认: 127.0.0.1)')
    parser.add_argument('port', nargs='?', type=int, default=6000, help='服务器端口 (默认: 6000)')
    parser.add_argument('clients', nargs='?', type=int, default=10, help='客户端数量 (默认: 10)')
    parser.add_argument('duration', nargs='?', type=int, default=30, help='测试时长(秒) (默认: 30)')
    
    args = parser.parse_args()
    
    if args.clients <= 0 or args.clients > 1000:
        logger.error("客户端数量应在 1-1000 之间")
        return 1
    
    if args.duration <= 0 or args.duration > 600:
        logger.error("测试时长应在 1-600 秒之间")
        return 1
    
    test = SimpleStressTest(args.host, args.port, args.clients, args.duration)
    
    try:
        success = await test.run_test()
        return 0 if success else 1
    except KeyboardInterrupt:
        logger.info("测试被用户中断")
        return 1
    except Exception as e:
        logger.error(f"测试执行失败: {e}")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("程序被用户中断")
        sys.exit(1)
