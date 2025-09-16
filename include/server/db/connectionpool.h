#ifndef CONNECTIONPOOL_H
#define CONNECTIONPOOL_H

#include <queue>
#include <string>
#include <memory>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>
#include <thread>
#include <mysql/mysql.h>

using namespace std;
using namespace chrono;

class Connection
{
public:
    Connection();
    ~Connection();
    
    // 连接数据库
    bool connect(const string& host, unsigned int port, const string& user, 
                const string& password, const string& dbname);
    
    // 更新操作 insert delete update
    bool update(const string& sql);
    
    // 查询操作 select
    MYSQL_RES* query(const string& sql);
    
    // 刷新连接的起始空闲时间点
    void refreshAliveTime() { _alivetime = steady_clock::now(); }
    
    // 返回存活的时间
    steady_clock::time_point getAliveTime() const { return _alivetime; }
    
    // 获取原始MySQL连接
    MYSQL* getMysqlConnection() { return _conn; }

private:
    MYSQL* _conn; // 表示和MySQL Server的一条连接
    steady_clock::time_point _alivetime; // 记录进入空闲状态后的起始存活时间
};

class ConnectionPool
{
public:
    // 获取连接池对象实例
    static ConnectionPool* getConnectionPool();
    
    //从连接池中获取一个可用的空闲连接接口
    shared_ptr<Connection> getConnection();
    
private:
    ConnectionPool(); // 
    
    bool loadConfigFile(); // 从配置文件中加载配置项
    
    void produceConnectionTask(); // 运行在独立的线程中，专门负责生产新连接
    
    void scanConnectionTask(); // 扫描超过maxIdleTime最大空闲时间连接，进行对应的连接回收
    
private:
    string _ip; // mysql的ip地址
    unsigned short _port; // mysql的端口号 3306
    string _username; // mysql登录用户名
    string _password; // mysql登录密码
    string _dbname; // 连接的数据库名称
    int _initSize; // 连接池的初始连接量
    int _maxSize; // 连接池的最大连接量
    int _maxIdleTime; // 连接池最大空闲时间
    int _connectionTimeOut; // 连接池获取连接的超时时间
    
    queue<Connection*> _connectionQue; // 存储mysql连接的队列
    mutex _queueMutex; // 维护连接队列的线程锁
    atomic_int _connectionCnt; // 队内连接数
    condition_variable cv; // 设置条件变量，用于连接生产线程和连接消费线程的通信
};

// RAII机制自动归还连接的包装类
class ConnectionRAII
{
public:
    // 构造时自动获取连接
    explicit ConnectionRAII(ConnectionPool* pool);
    
    // 禁止拷贝构造和赋值
    ConnectionRAII(const ConnectionRAII&) = delete;
    ConnectionRAII& operator=(const ConnectionRAII&) = delete;
    
    // 析构时自动归还连接
    ~ConnectionRAII();
    
    // 提供连接访问接口
    Connection* operator->() { return _conn.get(); }
    Connection& operator*() { return *_conn; }
    
    // 检查连接是否有效
    bool isValid() const { return _conn != nullptr; }
    
private:
    shared_ptr<Connection> _conn;
    ConnectionPool* _pool;
};

#endif