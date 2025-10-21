#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
智能聊天服务器压测工具
确保随机选择的用户ID一定存在
"""

import asyncio
import json
import time
import sys
import socket
import argparse
import logging
from typing import List, Optional, Dict
import random

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SmartChatClient:
    """智能聊天客户端"""
    
    def __init__(self, client_id: int, host: str, port: int):
        self.client_id = client_id
        self.host = host
        self.port = port
        self.user_id = None  # 动态分配
        self.username = None
        self.password = None
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.connected = False
        self.registered = False
        self.logged_in = False
        self.messages_sent = 0
        self.messages_received = 0
        self.available_targets: List[int] = []  # 可用的目标用户ID列表
        
    async def connect(self) -> bool:
        """连接到服务器"""
        try:
            self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
            self.connected = True
            logger.debug(f"客户端 {self.client_id} 连接成功")
            return True
        except Exception as e:
            logger.error(f"客户端 {self.client_id} 连接失败: {e}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self.writer:
            try:
                self.writer.close()
                try:
                    await asyncio.wait_for(self.writer.wait_closed(), timeout=1.0)
                except (ConnectionResetError, BrokenPipeError, OSError):
                    pass
                self.connected = False
                logger.debug(f"客户端 {self.client_id} 已断开连接")
            except Exception as e:
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
            return True
        except (ConnectionResetError, BrokenPipeError, OSError):
            self.connected = False
            return False
        except Exception as e:
            logger.error(f"客户端 {self.client_id} 发送消息失败: {e}")
            return False
    
    async def receive_message(self, timeout: float = 5.0) -> Optional[dict]:
        """接收消息"""
        if not self.reader:
            return None
        
        try:
            data = await asyncio.wait_for(self.reader.read(4096), timeout=timeout)
            if not data:
                return None
                
            message_str = data.decode('utf-8', errors='ignore').strip()
            if message_str:
                # 处理可能的多行JSON
                for line in message_str.split('\n'):
                    if line.strip():
                        try:
                            return json.loads(line)
                        except json.JSONDecodeError:
                            continue
                            
        except asyncio.TimeoutError:
            logger.debug(f"客户端 {self.client_id} 接收消息超时")
        except (ConnectionResetError, BrokenPipeError, OSError):
            self.connected = False
        except Exception as e:
            logger.warning(f"客户端 {self.client_id} 接收消息异常: {e}")
        
        return None
    
    async def register_user(self) -> bool:
        """注册用户"""
        self.username = f"user_{self.client_id:04d}"
        self.password = f"pass_{self.client_id:04d}"
        
        reg_msg = {
            "msgid": 3,  # REG_MSG
            "name": self.username,
            "password": self.password
        }
        
        if not await self.send_message(reg_msg):
            return False
        
        # 等待注册响应
        response = await self.receive_message()
        if response:
            if response.get('msgid') == 4 and response.get('errno') == 0:  # REG_MSG_ACK
                self.user_id = response.get('id')
                self.registered = True
                logger.info(f"✓ 客户端 {self.client_id} 注册成功: {self.username} (ID: {self.user_id})")
                return True
            else:
                logger.error(f"✗ 客户端 {self.client_id} 注册失败: {self.username}")
        
        return False
    
    async def login_user(self) -> bool:
        """登录用户"""
        if not self.user_id or not self.password:
            return False
        
        login_msg = {
            "msgid": 1,  # LOGIN_MSG
            "id": self.user_id,
            "password": self.password
        }
        
        if not await self.send_message(login_msg):
            return False
        
        # 等待登录响应
        response = await self.receive_message()
        if response:
            if response.get('msgid') == 2 and response.get('errno') == 0:  # LOGIN_MSG_ACK
                self.logged_in = True
                logger.debug(f"✓ 客户端 {self.client_id} 登录成功")
                return True
            else:
                logger.error(f"✗ 客户端 {self.client_id} 登录失败")
        
        return False
    
    def update_target_list(self, user_ids: List[int]):
        """更新可用的目标用户ID列表"""
        # 排除自己
        self.available_targets = [uid for uid in user_ids if uid != self.user_id]
    
    async def send_chat_message(self) -> bool:
        """发送聊天消息到随机存在的用户"""
        if not self.logged_in or not self.available_targets:
            return False
        
        # 随机选择一个确实存在的目标用户
        target_user = random.choice(self.available_targets)
        
        chat_msg = {
            "msgid": 5,  # ONE_CHAT_MSG
            "id": self.user_id,
            "from": self.user_id,  
            "to": target_user,
            "msg": f"测试消息来自用户{self.user_id}发给用户{target_user}",
            "time": str(int(time.time()))
        }
        
        success = await self.send_message(chat_msg)
        if success:
            self.messages_sent += 1
            logger.debug(f"客户端 {self.client_id} 发送消息: {self.user_id} -> {target_user}")
        
        return success
    
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
                
                # 简单计数
                message_str = data.decode('utf-8', errors='ignore')
                messages = message_str.strip().split('\n')
                self.messages_received += len([m for m in messages if m.strip()])
                
            except asyncio.TimeoutError:
                continue
            except (ConnectionResetError, BrokenPipeError, OSError):
                break
            except Exception as e:
                logger.warning(f"客户端 {self.client_id} 接收消息异常: {e}")
                break

class SmartStressTest:
    """智能压力测试工具"""
    
    def __init__(self, host: str, port: int, client_count: int, duration: int):
        self.host = host
        self.port = port
        self.client_count = client_count
        self.duration = duration
        self.clients: List[SmartChatClient] = []
        self.registered_user_ids: List[int] = []
        
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
    
    async def register_all_users(self) -> bool:
        """批量注册所有用户"""
        logger.info("阶段1: 批量注册用户")
        logger.info("=" * 40)
        
        # 创建客户端
        for i in range(self.client_count):
            client = SmartChatClient(i, self.host, self.port)
            self.clients.append(client)
        
        # 连接所有客户端
        logger.info("连接所有客户端...")
        connect_tasks = [client.connect() for client in self.clients]
        connect_results = await asyncio.gather(*connect_tasks, return_exceptions=True)
        connected_count = sum(1 for result in connect_results if result is True)
        
        logger.info(f"成功连接 {connected_count}/{self.client_count} 个客户端")
        
        if connected_count == 0:
            return False
        
        # 注册所有用户
        logger.info("批量注册用户...")
        register_tasks = []
        for client in self.clients:
            if client.connected:
                register_tasks.append(client.register_user())
        
        register_results = await asyncio.gather(*register_tasks, return_exceptions=True)
        registered_count = sum(1 for result in register_results if result is True)
        
        logger.info(f"成功注册 {registered_count}/{len(register_tasks)} 个用户")
        
        # 收集所有注册成功的用户ID
        self.registered_user_ids = [client.user_id for client in self.clients if client.registered]
        
        if len(self.registered_user_ids) < 2:
            logger.error("注册成功的用户数量太少，无法进行聊天压测")
            return False
        
        logger.info(f"用户池: {self.registered_user_ids}")
        
        # 更新所有客户端的目标用户列表
        for client in self.clients:
            if client.registered:
                client.update_target_list(self.registered_user_ids)
        
        # 断开注册连接（准备重新连接进行压测）
        disconnect_tasks = [client.disconnect() for client in self.clients]
        await asyncio.gather(*disconnect_tasks, return_exceptions=True)
        
        return True
    
    async def run_stress_test(self) -> bool:
        """运行压力测试"""
        logger.info("\n阶段2: 执行压力测试")
        logger.info("=" * 40)
        
        # 重新连接所有客户端
        logger.info("重新连接客户端...")
        connect_tasks = [client.connect() for client in self.clients if client.registered]
        connect_results = await asyncio.gather(*connect_tasks, return_exceptions=True)
        connected_count = sum(1 for result in connect_results if result is True)
        
        logger.info(f"成功连接 {connected_count} 个已注册客户端")
        
        # 登录所有用户
        logger.info("批量登录用户...")
        login_tasks = []
        for client in self.clients:
            if client.connected and client.registered:
                login_tasks.append(client.login_user())
        
        login_results = await asyncio.gather(*login_tasks, return_exceptions=True)
        logged_in_count = sum(1 for result in login_results if result is True)
        
        logger.info(f"成功登录 {logged_in_count}/{len(login_tasks)} 个用户")
        
        if logged_in_count < 2:
            logger.error("登录成功的用户数量太少")
            return False
        
        # 开始压测
        start_time = time.time()
        logger.info(f"开始压力测试，时长: {self.duration}秒")
        
        # 启动消息接收任务
        receive_tasks = []
        for client in self.clients:
            if client.logged_in:
                receive_tasks.append(client.receive_messages(self.duration))
        
        # 启动消息发送任务
        async def send_messages():
            end_time = time.time() + self.duration
            
            while time.time() < end_time:
                send_tasks = []
                for client in self.clients:
                    if client.logged_in and client.available_targets:
                        send_tasks.append(client.send_chat_message())
                
                if send_tasks:
                    await asyncio.gather(*send_tasks, return_exceptions=True)
                
                # 控制发送频率
                await asyncio.sleep(0.5)
        
        # 同时运行发送和接收任务
        try:
            await asyncio.gather(send_messages(), *receive_tasks)
        except Exception as e:
            logger.error(f"压测过程中出现异常: {e}")
        
        # 计算结果
        end_time = time.time()
        actual_duration = end_time - start_time
        
        total_sent = sum(client.messages_sent for client in self.clients)
        total_received = sum(client.messages_received for client in self.clients)
        active_clients = sum(1 for client in self.clients if client.logged_in)
        
        # 断开所有连接
        logger.info("断开所有连接...")
        disconnect_tasks = [client.disconnect() for client in self.clients]
        await asyncio.gather(*disconnect_tasks, return_exceptions=True)
        
        # 生成报告
        self.generate_report(actual_duration, active_clients, total_sent, total_received)
        
        return True
    
    async def run_full_test(self) -> bool:
        """运行完整测试流程"""
        logger.info("智能聊天服务器压力测试")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"客户端数量: {self.client_count}")
        logger.info(f"测试时长: {self.duration}秒")
        logger.info("特性: 确保随机目标用户ID一定存在")
        
        if not self.check_server():
            logger.error(f"无法连接到服务器 {self.host}:{self.port}")
            return False
        
        # 阶段1: 注册用户
        if not await self.register_all_users():
            logger.error("用户注册阶段失败")
            return False
        
        # 等待一下
        await asyncio.sleep(2)
        
        # 阶段2: 压力测试
        if not await self.run_stress_test():
            logger.error("压力测试阶段失败")
            return False
        
        return True
    
    def generate_report(self, duration: float, active: int, sent: int, received: int):
        """生成测试报告"""
        logger.info("\n" + "=" * 50)
        logger.info("智能压力测试报告")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"测试时长: {duration:.2f}秒")
        logger.info(f"目标客户端数: {self.client_count}")
        logger.info(f"成功注册用户数: {len(self.registered_user_ids)}")
        logger.info(f"活跃测试客户端: {active}")
        logger.info(f"用户池大小: {len(self.registered_user_ids)}")
        logger.info(f"总发送消息数: {sent}")
        logger.info(f"总接收消息数: {received}")
        logger.info(f"平均QPS: {sent/duration:.2f}")
        logger.info(f"每客户端平均发送: {sent/max(active, 1):.1f}")
        logger.info(f"消息成功率: {(received/max(sent, 1))*100:.1f}%")
        logger.info("=" * 50)
        logger.info("✓ 所有聊天消息的目标用户ID都确实存在")

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='智能聊天服务器压力测试 - 确保用户ID存在')
    parser.add_argument('host', nargs='?', default='127.0.0.1', help='服务器IP (默认: 127.0.0.1)')
    parser.add_argument('port', nargs='?', type=int, default=6000, help='服务器端口 (默认: 6000)')
    parser.add_argument('clients', nargs='?', type=int, default=10, help='客户端数量 (默认: 10)')
    parser.add_argument('duration', nargs='?', type=int, default=30, help='测试时长(秒) (默认: 30)')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细日志')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    if args.clients <= 0 or args.clients > 1000:
        logger.error("客户端数量应在 1-1000 之间")
        return 1
    
    if args.duration <= 0 or args.duration > 600:
        logger.error("测试时长应在 1-600 秒之间")
        return 1
    
    test = SmartStressTest(args.host, args.port, args.clients, args.duration)
    
    try:
        success = await test.run_full_test()
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