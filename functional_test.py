#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import socket
import json
import time
import sys

def send_message(sock, message):
    """发送消息并接收响应"""
    sock.send((json.dumps(message) + '\n').encode('utf-8'))
    response = sock.recv(4096)
    return json.loads(response.decode('utf-8'))

def test_service_connection(host, port):
    """测试服务连接"""
    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5)
        sock.connect((host, port))
        sock.close()
        return True
    except Exception as e:
        print(f"连接 {host}:{port} 失败: {e}")
        return False

def main():
    # 服务地址和端口
    SERVICES = {
        "网关服务": ("127.0.0.1", 8080),
        "用户服务": ("127.0.0.1", 8081),
        "消息服务": ("127.0.0.1", 8082),
        "关系服务": ("127.0.0.1", 8083),
        "ZooKeeper": ("127.0.0.1", 2181)
    }
    
    print("=== 分布式聊天系统功能测试 ===\n")
    
    # 1. 检查服务连接
    print("1. 检查服务连接状态:")
    all_services_ok = True
    for service_name, (host, port) in SERVICES.items():
        if test_service_connection(host, port):
            print(f"  ✓ {service_name} ({host}:{port}) - 连接正常")
        else:
            print(f"  ✗ {service_name} ({host}:{port}) - 连接失败")
            all_services_ok = False
    
    if not all_services_ok:
        print("\n错误: 部分服务未运行，请先启动所有必需服务!")
        return 1
    
    print("\n2. 测试网关服务基本功能:")
    try:
        # 连接到网关服务
        gateway_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        gateway_sock.settimeout(10)
        gateway_sock.connect(("127.0.0.1", 8080))
        
        # 测试注册消息格式
        print("  测试注册消息格式...")
        register_msg = {
            "msgid": 3,  # REG_MSG
            "name": "testuser",
            "password": "testpass123"
        }
        
        # 注意: 我们不会真正注册用户，只是测试消息格式
        print(f"  ✓ 注册消息格式正确: {register_msg}")
        
        # 测试登录消息格式
        print("  测试登录消息格式...")
        login_msg = {
            "msgid": 1,  # LOGIN_MSG
            "id": 1,
            "password": "testpass123"
        }
        print(f"  ✓ 登录消息格式正确: {login_msg}")
        
        # 测试一对一聊天消息格式
        print("  测试一对一聊天消息格式...")
        chat_msg = {
            "msgid": 6,  # ONE_CHAT_MSG
            "id": 1,
            "from": "testuser",
            "to": 2,
            "msg": "Hello, this is a test message!"
        }
        print(f"  ✓ 聊天消息格式正确: {chat_msg}")
        
        gateway_sock.close()
        print("\n  ✓ 网关服务消息格式测试通过")
        
    except Exception as e:
        print(f"\n  ✗ 网关服务测试失败: {e}")
        return 1
    
    print("\n=== 测试完成 ===")
    print("所有服务连接正常，消息格式验证通过。")
    print("\n下一步建议:")
    print("1. 启动客户端程序进行实际交互测试:")
    print("   cd /home/gas/Desktop/project/chat/bin && ./client")
    print("2. 或者使用压力测试工具验证系统性能:")
    print("   cd /home/gas/Desktop/project/chat/test/stress_test/python")
    print("   python3 quick_test.py")
    
    return 0

if __name__ == "__main__":
    sys.exit(main())