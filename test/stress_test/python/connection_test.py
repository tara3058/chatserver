#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
聊天服务器连接测试工具 - Python版本
用于验证服务器基本功能
"""

import asyncio
import json
import time
import argparse
import logging

# 配置日志
logging.basicConfig(
    level=logging.INFO,
    format='%(asctime)s - %(levelname)s - %(message)s'
)
logger = logging.getLogger(__name__)

class SimpleTestClient:
    """简单测试客户端"""
    
    def __init__(self, host: str, port: int):
        self.host = host
        self.port = port
        self.reader = None
        self.writer = None
        self.user_id = 1001
        
    async def connect(self) -> bool:
        """连接到服务器"""
        try:
            logger.info(f"正在连接到 {self.host}:{self.port}...")
            self.reader, self.writer = await asyncio.open_connection(self.host, self.port)
            logger.info("连接成功!")
            return True
        except Exception as e:
            logger.error(f"连接失败: {e}")
            return False
    
    async def disconnect(self):
        """断开连接"""
        if self.writer:
            try:
                self.writer.close()
                await self.writer.wait_closed()
                logger.info("连接已断开")
            except Exception as e:
                logger.error(f"断开连接时出错: {e}")
    
    async def send_message(self, message: dict) -> bool:
        """发送消息"""
        if not self.writer:
            logger.error("未连接到服务器")
            return False
        
        try:
            msg_str = json.dumps(message, ensure_ascii=False)
            logger.info(f"发送消息: {msg_str}")
            self.writer.write(msg_str.encode('utf-8'))
            await self.writer.drain()
            return True
        except Exception as e:
            logger.error(f"发送消息失败: {e}")
            return False
    
    async def receive_message(self, timeout: float = 5.0) -> dict:
        """接收消息"""
        if not self.reader:
            return None
        
        try:
            data = await asyncio.wait_for(self.reader.read(4096), timeout=timeout)
            if data:
                message_str = data.decode('utf-8').strip()
                logger.info(f"接收消息: {message_str}")
                return json.loads(message_str)
        except asyncio.TimeoutError:
            logger.warning("接收消息超时")
        except Exception as e:
            logger.error(f"接收消息失败: {e}")
        
        return None
    
    async def test_login(self) -> bool:
        """测试登录功能"""
        logger.info("=" * 40)
        logger.info("测试登录功能")
        logger.info("=" * 40)
        
        # 发送登录消息
        login_msg = {
            "msgid": 1,
            "id": self.user_id,
            "password": "123456"
        }
        
        if not await self.send_message(login_msg):
            return False
        
        # 等待响应
        response = await self.receive_message()
        if response:
            msgid = response.get('msgid', 0)
            errno = response.get('errno', -1)
            
            if msgid == 1 and errno == 0:
                logger.info("✓ 登录测试成功")
                return True
            else:
                logger.error(f"✗ 登录失败 - 错误码: {errno}")
                if 'errmsg' in response:
                    logger.error(f"错误信息: {response['errmsg']}")
        else:
            logger.error("✗ 未收到登录响应")
        
        return False
    
    async def test_chat_message(self) -> bool:
        """测试聊天消息功能"""
        logger.info("=" * 40)
        logger.info("测试聊天消息功能")
        logger.info("=" * 40)
        
        # 发送聊天消息
        chat_msg = {
            "msgid": 5,
            "id": self.user_id,
            "from": self.user_id,
            "to": 1002,
            "msg": "这是一条测试消息",
            "time": str(int(time.time()))
        }
        
        if not await self.send_message(chat_msg):
            return False
        
        # 等待响应
        response = await self.receive_message()
        if response:
            msgid = response.get('msgid', 0)
            errno = response.get('errno', -1)
            
            if errno == 0:
                logger.info("✓ 聊天消息测试成功")
                return True
            else:
                logger.error(f"✗ 聊天消息发送失败 - 错误码: {errno}")
                if 'errmsg' in response:
                    logger.error(f"错误信息: {response['errmsg']}")
        else:
            logger.warning("⚠ 未收到聊天消息响应 (可能是正常现象)")
            return True  # 聊天消息可能不需要响应
        
        return False
    
    async def test_group_message(self) -> bool:
        """测试群聊消息功能"""
        logger.info("=" * 40)
        logger.info("测试群聊消息功能")
        logger.info("=" * 40)
        
        # 发送群聊消息
        group_msg = {
            "msgid": 7,  # GROUP_CHAT_MSG
            "id": self.user_id,
            "from": self.user_id,
            "groupid": 1,
            "msg": "这是一条群聊测试消息",
            "time": str(int(time.time()))
        }
        
        if not await self.send_message(group_msg):
            return False
        
        # 等待响应
        response = await self.receive_message()
        if response:
            msgid = response.get('msgid', 0)
            errno = response.get('errno', -1)
            
            if errno == 0:
                logger.info("✓ 群聊消息测试成功")
                return True
            else:
                logger.error(f"✗ 群聊消息发送失败 - 错误码: {errno}")
                if 'errmsg' in response:
                    logger.error(f"错误信息: {response['errmsg']}")
        else:
            logger.warning("⚠ 未收到群聊消息响应")
        
        return False
    
    async def run_all_tests(self):
        """运行所有测试"""
        logger.info("开始聊天服务器功能测试")
        logger.info(f"目标服务器: {self.host}:{self.port}")
        
        # 连接测试
        if not await self.connect():
            logger.error("连接测试失败，终止测试")
            return False
        
        test_results = []
        
        try:
            # 登录测试
            login_result = await self.test_login()
            test_results.append(("登录功能", login_result))
            
            if login_result:
                # 等待一下再进行其他测试
                await asyncio.sleep(1)
                
                # 聊天消息测试
                chat_result = await self.test_chat_message()
                test_results.append(("聊天消息", chat_result))
                
                await asyncio.sleep(1)
                
                # 群聊消息测试
                group_result = await self.test_group_message()
                test_results.append(("群聊消息", group_result))
            else:
                logger.warning("登录失败，跳过其他测试")
        
        except Exception as e:
            logger.error(f"测试过程中出现异常: {e}")
        
        finally:
            await self.disconnect()
        
        # 生成测试报告
        self.generate_test_report(test_results)
        
        return all(result for _, result in test_results)
    
    def generate_test_report(self, test_results):
        """生成测试报告"""
        logger.info("=" * 50)
        logger.info("测试报告")
        logger.info("=" * 50)
        
        total_tests = len(test_results)
        passed_tests = sum(1 for _, result in test_results if result)
        
        logger.info(f"总测试数: {total_tests}")
        logger.info(f"通过测试: {passed_tests}")
        logger.info(f"失败测试: {total_tests - passed_tests}")
        logger.info(f"成功率: {passed_tests/total_tests*100:.1f}%")
        
        logger.info("\n详细结果:")
        for test_name, result in test_results:
            status = "✓ 通过" if result else "✗ 失败"
            logger.info(f"  {test_name}: {status}")
        
        logger.info("=" * 50)
        
        if passed_tests == total_tests:
            logger.info("🎉 所有测试通过！服务器功能正常")
        else:
            logger.warning("⚠️  部分测试失败，请检查服务器状态")

async def main():
    """主函数"""
    parser = argparse.ArgumentParser(description='聊天服务器连接测试工具')
    parser.add_argument('host', help='服务器IP地址')
    parser.add_argument('port', type=int, help='服务器端口')
    parser.add_argument('--user-id', type=int, default=1001, help='测试用户ID (默认: 1001)')
    parser.add_argument('--log-level', default='INFO',
                       choices=['DEBUG', 'INFO', 'WARNING', 'ERROR'],
                       help='日志级别 (默认: INFO)')
    
    args = parser.parse_args()
    
    # 设置日志级别
    logging.getLogger().setLevel(getattr(logging, args.log_level))
    
    # 创建测试客户端
    client = SimpleTestClient(args.host, args.port)
    client.user_id = args.user_id
    
    # 运行测试
    try:
        success = await client.run_all_tests()
        return 0 if success else 1
    except KeyboardInterrupt:
        logger.info("测试被用户中断")
        return 1
    except Exception as e:
        logger.error(f"测试执行失败: {e}")
        return 1

if __name__ == "__main__":
    import sys
    try:
        exit_code = asyncio.run(main())
        sys.exit(exit_code)
    except KeyboardInterrupt:
        logger.info("测试被用户中断")
        sys.exit(1)