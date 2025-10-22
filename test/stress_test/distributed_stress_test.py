# test/stress_test/distributed_stress_test.py
import asyncio
import aiohttp
import time
import random
import json
from concurrent.futures import ThreadPoolExecutor
import threading

class DistributedStressTester:
    def __init__(self, gateway_address, user_count=1000, message_count=10000):
        self.gateway_address = gateway_address
        self.user_count = user_count
        self.message_count = message_count
        self.session = None
        self.stats = {
            'total_requests': 0,
            'successful_requests': 0,
            'failed_requests': 0,
            'total_latency': 0,
            'start_time': None,
            'end_time': None
        }
        
    async def create_session(self):
        """创建HTTP会话"""
        self.session = aiohttp.ClientSession()
        
    async def close_session(self):
        """关闭HTTP会话"""
        if self.session:
            await self.session.close()
            
    async def register_user(self, user_id):
        """注册用户"""
        try:
            url = f"http://{self.gateway_address}/register"
            data = {
                "name": f"user_{user_id}",
                "password": f"password_{user_id}"
            }
            
            start_time = time.time()
            async with self.session.post(url, json=data) as response:
                latency = (time.time() - start_time) * 1000
                self.stats['total_requests'] += 1
                self.stats['total_latency'] += latency
                
                if response.status == 200:
                    self.stats['successful_requests'] += 1
                    return await response.json()
                else:
                    self.stats['failed_requests'] += 1
                    return None
        except Exception as e:
            self.stats['failed_requests'] += 1
            print(f"Error registering user {user_id}: {e}")
            return None
            
    async def login_user(self, user_id):
        """用户登录"""
        try:
            url = f"http://{self.gateway_address}/login"
            data = {
                "id": user_id,
                "password": f"password_{user_id}"
            }
            
            start_time = time.time()
            async with self.session.post(url, json=data) as response:
                latency = (time.time() - start_time) * 1000
                self.stats['total_requests'] += 1
                self.stats['total_latency'] += latency
                
                if response.status == 200:
                    self.stats['successful_requests'] += 1
                    return await response.json()
                else:
                    self.stats['failed_requests'] += 1
                    return None
        except Exception as e:
            self.stats['failed_requests'] += 1
            print(f"Error logging in user {user_id}: {e}")
            return None
            
    async def send_message(self, from_id, to_id, message):
        """发送消息"""
        try:
            url = f"http://{self.gateway_address}/send_message"
            data = {
                "from_id": from_id,
                "to_id": to_id,
                "message": message
            }
            
            start_time = time.time()
            async with self.session.post(url, json=data) as response:
                latency = (time.time() - start_time) * 1000
                self.stats['total_requests'] += 1
                self.stats['total_latency'] += latency
                
                if response.status == 200:
                    self.stats['successful_requests'] += 1
                    return await response.json()
                else:
                    self.stats['failed_requests'] += 1
                    return None
        except Exception as e:
            self.stats['failed_requests'] += 1
            print(f"Error sending message from {from_id} to {to_id}: {e}")
            return None
            
    async def run_test(self):
        """运行压力测试"""
        print(f"Starting distributed stress test with {self.user_count} users and {self.message_count} messages")
        
        self.stats['start_time'] = time.time()
        
        # 创建会话
        await self.create_session()
        
        try:
            # 1. 注册用户
            print("Registering users...")
            register_tasks = [self.register_user(i) for i in range(1, self.user_count + 1)]
            register_results = await asyncio.gather(*register_tasks, return_exceptions=True)
            
            # 2. 用户登录
            print("Logging in users...")
            login_tasks = [self.login_user(i) for i in range(1, self.user_count + 1)]
            login_results = await asyncio.gather(*login_tasks, return_exceptions=True)
            
            # 3. 发送消息
            print("Sending messages...")
            message_tasks = []
            for _ in range(self.message_count):
                from_id = random.randint(1, self.user_count)
                to_id = random.randint(1, self.user_count)
                while to_id == from_id:
                    to_id = random.randint(1, self.user_count)
                
                message = f"Test message {_}"
                message_tasks.append(self.send_message(from_id, to_id, message))
            
            message_results = await asyncio.gather(*message_tasks, return_exceptions=True)
            
        finally:
            # 关闭会话
            await self.close_session()
            
        self.stats['end_time'] = time.time()
        
        # 打印统计信息
        self.print_stats()
        
    def print_stats(self):
        """打印统计信息"""
        duration = self.stats['end_time'] - self.stats['start_time']
        avg_latency = self.stats['total_latency'] / max(self.stats['total_requests'], 1)
        qps = self.stats['total_requests'] / max(duration, 0.001)
        
        print("\n=== Stress Test Results ===")
        print(f"Total Requests: {self.stats['total_requests']}")
        print(f"Successful Requests: {self.stats['successful_requests']}")
        print(f"Failed Requests: {self.stats['failed_requests']}")
        print(f"Success Rate: {self.stats['successful_requests'] / max(self.stats['total_requests'], 1) * 100:.2f}%")
        print(f"Average Latency: {avg_latency:.2f} ms")
        print(f"Total Duration: {duration:.2f} seconds")
        print(f"QPS: {qps:.2f}")

# 运行测试
async def main():
    tester = DistributedStressTester("127.0.0.1:8080", user_count=100, message_count=1000)
    await tester.run_test()

if __name__ == "__main__":
    asyncio.run(main())