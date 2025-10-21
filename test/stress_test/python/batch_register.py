#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
聊天服务器批量注册工具
支持批量创建多个用户账号，用于压测和开发测试
"""

import asyncio
import json
import time
import sys
import socket
import argparse
import logging
from typing import List, Optional, Tuple
import random
import string

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class BatchRegisterClient:
    """批量注册客户端"""
    
    def __init__(self, client_id: int, host: str, port: int):
        self.client_id = client_id
        self.host = host
        self.port = port
        self.reader: Optional[asyncio.StreamReader] = None
        self.writer: Optional[asyncio.StreamWriter] = None
        self.connected = False
        self.registration_success = False
        self.user_info = None
        
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
                return json.loads(message_str)
                
        except asyncio.TimeoutError:
            logger.warning(f"客户端 {self.client_id} 接收消息超时")
        except (ConnectionResetError, BrokenPipeError, OSError):
            self.connected = False
        except json.JSONDecodeError as e:
            logger.error(f"客户端 {self.client_id} JSON解析失败: {e}")
        except Exception as e:
            logger.warning(f"客户端 {self.client_id} 接收消息异常: {e}")
        
        return None
    
    async def register_user(self, username: str, password: str) -> Tuple[bool, Optional[int]]:
        """注册用户"""
        reg_msg = {
            "msgid": 3,  # REG_MSG
            "name": username,
            "password": password
        }
        
        if not await self.send_message(reg_msg):
            logger.error(f"发送注册消息失败: {username}")
            return False, None
        
        logger.debug(f"发送注册请求: {username}")
        
        # 等待注册响应
        response = await self.receive_message()
        if response:
            msgid = response.get('msgid', 0)
            errno = response.get('errno', -1)
            
            if msgid == 4 and errno == 0:  # REG_MSG_ACK
                user_id = response.get('id', -1)
                self.registration_success = True
                self.user_info = {
                    'id': user_id,
                    'name': username,
                    'password': password
                }
                logger.info(f"✓ 用户注册成功: {username} (ID: {user_id})")
                return True, user_id
            else:
                error_msg = response.get('errmsg', '未知错误')
                logger.error(f"✗ 用户注册失败: {username} - 错误码: {errno}, 消息: {error_msg}")
                return False, None
        else:
            logger.error(f"✗ 未收到注册响应: {username}")
            return False, None

class BatchRegisterTool:
    """批量注册工具"""
    
    def __init__(self, host: str, port: int, user_count: int, username_prefix: str = "testuser"):
        self.host = host
        self.port = port
        self.user_count = user_count
        self.username_prefix = username_prefix
        self.clients: List[BatchRegisterClient] = []
        self.successful_registrations: List[dict] = []
        self.failed_registrations: List[str] = []
    
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
    
    def generate_username(self, index: int, use_random: bool = False) -> str:
        """生成用户名"""
        if use_random:
            # 生成随机用户名
            random_suffix = ''.join(random.choices(string.ascii_lowercase + string.digits, k=6))
            return f"{self.username_prefix}_{random_suffix}"
        else:
            # 生成有序用户名
            return f"{self.username_prefix}_{index:04d}"
    
    def generate_password(self, username: str) -> str:
        """生成密码"""
        # 简单密码生成策略，实际项目中可以使用更复杂的策略
        return f"{username}_pass123"
    
    async def register_single_user(self, client_id: int, username: str, password: str) -> bool:
        """注册单个用户"""
        client = BatchRegisterClient(client_id, self.host, self.port)
        
        try:
            # 连接服务器
            if not await client.connect():
                self.failed_registrations.append(f"{username} - 连接失败")
                return False
            
            # 执行注册
            success, user_id = await client.register_user(username, password)
            
            if success:
                self.successful_registrations.append({
                    'id': user_id,
                    'username': username,
                    'password': password
                })
                return True
            else:
                self.failed_registrations.append(f"{username} - 注册失败")
                return False
                
        except Exception as e:
            logger.error(f"注册用户 {username} 时发生异常: {e}")
            self.failed_registrations.append(f"{username} - 异常: {e}")
            return False
        finally:
            await client.disconnect()
    
    async def batch_register_sequential(self, use_random_names: bool = False) -> bool:
        """顺序批量注册"""
        logger.info("=" * 50)
        logger.info("开始顺序批量注册")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"注册用户数量: {self.user_count}")
        logger.info(f"用户名前缀: {self.username_prefix}")
        logger.info(f"随机用户名: {'是' if use_random_names else '否'}")
        
        if not self.check_server():
            logger.error(f"无法连接到服务器 {self.host}:{self.port}")
            return False
        
        start_time = time.time()
        
        for i in range(self.user_count):
            username = self.generate_username(i, use_random_names)
            password = self.generate_password(username)
            
            logger.info(f"正在注册用户 {i+1}/{self.user_count}: {username}")
            
            success = await self.register_single_user(i, username, password)
            
            # 添加小延迟避免过快请求
            await asyncio.sleep(0.1)
        
        end_time = time.time()
        duration = end_time - start_time
        
        self.generate_report(duration)
        return True
    
    async def batch_register_concurrent(self, concurrent_limit: int = 10, use_random_names: bool = False) -> bool:
        """并发批量注册"""
        logger.info("=" * 50)
        logger.info("开始并发批量注册")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"注册用户数量: {self.user_count}")
        logger.info(f"并发限制: {concurrent_limit}")
        logger.info(f"用户名前缀: {self.username_prefix}")
        logger.info(f"随机用户名: {'是' if use_random_names else '否'}")
        
        if not self.check_server():
            logger.error(f"无法连接到服务器 {self.host}:{self.port}")
            return False
        
        start_time = time.time()
        
        # 创建用户信息
        user_infos = []
        for i in range(self.user_count):
            username = self.generate_username(i, use_random_names)
            password = self.generate_password(username)
            user_infos.append((i, username, password))
        
        # 分批并发注册
        semaphore = asyncio.Semaphore(concurrent_limit)
        
        async def register_with_semaphore(client_id: int, username: str, password: str):
            async with semaphore:
                return await self.register_single_user(client_id, username, password)
        
        # 创建所有注册任务
        tasks = [
            register_with_semaphore(client_id, username, password)
            for client_id, username, password in user_infos
        ]
        
        logger.info(f"开始并发注册 {len(tasks)} 个用户...")
        
        # 执行所有注册任务
        results = await asyncio.gather(*tasks, return_exceptions=True)
        
        end_time = time.time()
        duration = end_time - start_time
        
        self.generate_report(duration)
        return True
    
    def generate_report(self, duration: float):
        """生成注册报告"""
        logger.info("=" * 50)
        logger.info("批量注册报告")
        logger.info("=" * 50)
        logger.info(f"服务器: {self.host}:{self.port}")
        logger.info(f"注册时长: {duration:.2f}秒")
        logger.info(f"目标用户数: {self.user_count}")
        logger.info(f"成功注册数: {len(self.successful_registrations)}")
        logger.info(f"失败注册数: {len(self.failed_registrations)}")
        logger.info(f"注册成功率: {len(self.successful_registrations)/self.user_count*100:.1f}%")
        logger.info(f"平均注册速度: {len(self.successful_registrations)/duration:.2f} 用户/秒")
        
        if self.successful_registrations:
            logger.info("\n成功注册的用户:")
            for user in self.successful_registrations:
                logger.info(f"  ID: {user['id']}, 用户名: {user['username']}, 密码: {user['password']}")
        
        if self.failed_registrations:
            logger.info("\n失败的注册:")
            for failure in self.failed_registrations:
                logger.info(f"  {failure}")
        
        logger.info("=" * 50)
    
    def save_user_info(self, filename: str = "registered_users.json"):
        """保存注册成功的用户信息到文件"""
        if self.successful_registrations:
            try:
                with open(filename, 'w', encoding='utf-8') as f:
                    json.dump(self.successful_registrations, f, ensure_ascii=False, indent=2)
                logger.info(f"用户信息已保存到: {filename}")
            except Exception as e:
                logger.error(f"保存用户信息失败: {e}")

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='聊天服务器批量注册工具')
    parser.add_argument('host', nargs='?', default='127.0.0.1', help='服务器IP (默认: 127.0.0.1)')
    parser.add_argument('port', nargs='?', type=int, default=6000, help='服务器端口 (默认: 6000)')
    parser.add_argument('count', nargs='?', type=int, default=10, help='注册用户数量 (默认: 10)')
    parser.add_argument('--prefix', default='testuser', help='用户名前缀 (默认: testuser)')
    parser.add_argument('--concurrent', '-c', type=int, default=0, help='并发数量，0表示顺序注册 (默认: 0)')
    parser.add_argument('--random-names', '-r', action='store_true', help='使用随机用户名')
    parser.add_argument('--save-file', '-s', default='registered_users.json', help='保存用户信息的文件名')
    parser.add_argument('--verbose', '-v', action='store_true', help='详细日志')
    
    args = parser.parse_args()
    
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
    
    if args.count <= 0 or args.count > 10000:
        logger.error("用户数量应在 1-10000 之间")
        return 1
    
    if args.concurrent < 0 or args.concurrent > 100:
        logger.error("并发数量应在 0-100 之间")
        return 1
    
    tool = BatchRegisterTool(args.host, args.port, args.count, args.prefix)
    
    try:
        if args.concurrent > 0:
            success = await tool.batch_register_concurrent(args.concurrent, args.random_names)
        else:
            success = await tool.batch_register_sequential(args.random_names)
        
        if success:
            tool.save_user_info(args.save_file)
        
        return 0 if success else 1
        
    except KeyboardInterrupt:
        logger.info("注册被用户中断")
        return 1
    except Exception as e:
        logger.error(f"注册执行失败: {e}")
        return 1

if __name__ == "__main__":
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("程序被用户中断")
        sys.exit(1)