#include "connectionpool.h"
#include <iostream>
#include <thread>
#include <vector>
#include <chrono>

using namespace std;

void testConnectionPool()
{
    cout << "开始测试数据库连接池..." << endl;
    
    // 测试1：获取单个连接
    cout << "\n=== 测试1：获取单个连接 ===" << endl;
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    shared_ptr<Connection> conn = pool->getConnection();
    
    if (conn != nullptr)
    {
        cout << "✓ 成功获取数据库连接" << endl;
        
        // 测试简单查询
        MYSQL_RES* res = conn->query("SELECT 1 as test_value");
        if (res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row != nullptr)
            {
                cout << "✓ 查询测试成功，返回值: " << row[0] << endl;
            }
            mysql_free_result(res);
        }
        else
        {
            cout << "✗ 查询测试失败" << endl;
        }
    }
    else
    {
        cout << "✗ 获取数据库连接失败" << endl;
        return;
    }
    
    // 测试2：多线程并发获取连接
    cout << "\n=== 测试2：多线程并发获取连接 ===" << endl;
    const int threadCount = 5;
    vector<thread> threads;
    
    auto worker = [](int threadId) {
        ConnectionPool* pool = ConnectionPool::getConnectionPool();
        shared_ptr<Connection> conn = pool->getConnection();
        
        if (conn != nullptr)
        {
            cout << "线程 " << threadId << " 成功获取连接" << endl;
            
            // 模拟数据库操作
            this_thread::sleep_for(chrono::milliseconds(100));
            
            MYSQL_RES* res = conn->query("SELECT CONNECTION_ID()");
            if (res != nullptr)
            {
                MYSQL_ROW row = mysql_fetch_row(res);
                if (row != nullptr)
                {
                    cout << "线程 " << threadId << " 连接ID: " << row[0] << endl;
                }
                mysql_free_result(res);
            }
        }
        else
        {
            cout << "线程 " << threadId << " 获取连接失败" << endl;
        }
    };
    
    // 启动多个线程
    for (int i = 0; i < threadCount; ++i)
    {
        threads.emplace_back(worker, i + 1);
    }
    
    // 等待所有线程完成
    for (auto& t : threads)
    {
        t.join();
    }
    
    cout << "\n=== 连接池测试完成 ===" << endl;
}

// 测试连接池配置和基本功能
void testConnectionPoolConfig()
{
    cout << "\n=== 测试连接池配置 ===" << endl;
    
    // 获取多个连接来测试连接池大小限制
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    vector<shared_ptr<Connection>> connections;
    
    // 尝试获取15个连接（初始大小为10）
    for (int i = 0; i < 15; ++i)
    {
        auto conn = pool->getConnection();
        if (conn != nullptr)
        {
            connections.push_back(conn);
            cout << "成功获取第 " << (i + 1) << " 个连接" << endl;
        }
        else
        {
            cout << "获取第 " << (i + 1) << " 个连接失败" << endl;
            break;
        }
    }
    
    cout << "总共获取了 " << connections.size() << " 个连接" << endl;
    
    // 释放所有连接（通过shared_ptr自动释放）
    connections.clear();
    cout << "所有连接已释放回连接池" << endl;
}

int main()
{
    try
    {
        testConnectionPool();
        testConnectionPoolConfig();
        
        cout << "\n所有测试完成！" << endl;
    }
    catch (const exception& e)
    {
        cout << "测试过程中发生异常: " << e.what() << endl;
        return -1;
    }
    
    return 0;
}