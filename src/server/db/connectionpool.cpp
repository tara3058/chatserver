#include "connectionpool.h"
#include <fstream>
#include <iostream>
#include <functional>
#include <cstdio>
#include <cstring>
#include <muduo/base/Logging.h>

using namespace std;

// 初始化数据库链接
Connection::Connection()
{
    _conn = mysql_init(nullptr);
}

Connection::~Connection()
{
    if (_conn != nullptr)
        mysql_close(_conn);
}
// 链接数据库
bool Connection::connect(const string& host, unsigned int port, const string& user, 
                        const string& password, const string& dbname)
{
    MYSQL* p = mysql_real_connect(_conn, host.c_str(), user.c_str(), 
                                 password.c_str(), dbname.c_str(), port, nullptr, 0);
    if (p != nullptr)
    {
        mysql_query(_conn, "set names utf8mb4");
        LOG_INFO << "connect mysql success!";
        return true;
    }
    else
    {
        LOG_ERROR << "connect mysql fail! Error: " << mysql_error(_conn);
        return false;
    }
}
// 更新操作
bool Connection::update(const string& sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_ERROR << "update fail: " << sql << " error: " << mysql_error(_conn);
        return false;
    }
    return true;
}

MYSQL_RES* Connection::query(const string& sql)
{
    if (mysql_query(_conn, sql.c_str()))
    {
        LOG_ERROR << "query fail: " << sql << " error: " << mysql_error(_conn);
        return nullptr;
    }
    return mysql_use_result(_conn);
}

/*ConnectionPool类实现
*/
// 连接池接口
ConnectionPool* ConnectionPool::getConnectionPool()
{
    static ConnectionPool pool; // lock和unlock
    return &pool;
}
// 加载配置文件
bool ConnectionPool::loadConfigFile()
{
    FILE *pf = fopen("mysql.conf", "r");
    if (pf == nullptr)
    {
        LOG_ERROR << "mysql.conf file is not exist!";
        // 设置默认配置
        _ip = "127.0.0.1";
        _port = 3306;
        _username = "root";
        _password = "123456";
        _dbname = "chat";
        _initSize = 10;        // 初始连接数量
        _maxSize = 1024;       // 最大连接数量
        _maxIdleTime = 60;     // 最大空闲时间60秒
        _connectionTimeOut = 10; // 获取连接超时时间10秒
        return true;
    }
    
    char line[1024];
    while (fgets(line, sizeof(line), pf) != nullptr)
    {
        string str = line;
        int idx = str.find('=', 0);
        // 无效配置项
        if (idx == -1)
        {
            LOG_INFO << "无效配置项: " << str;
            continue;
        }

        int endix = str.find('\n', idx);
        string key = str.substr(0, idx);
        string value = str.substr(idx + 1, endix - 1 - idx);

        if (key == "ip")
        {
            LOG_INFO << "ip: " << value;
            _ip = value;
        }
        else if (key == "port")
        {
            LOG_INFO << "port: " << value;
            _port = atoi(value.c_str());
        }
        else if (key == "username")
        {
            LOG_INFO << "username: " << value;
            _username = value;
        }
        else if (key == "password")
        {
            LOG_INFO << "password: " << value;
            _password = value;
        }
        else if (key == "dbname")
        {
            LOG_INFO << "dbname: " << value;
            _dbname = value;
        }
        else if (key == "initSize")
        {
            LOG_INFO << "initSize: " << value;
            _initSize = atoi(value.c_str());
        }
        else if (key == "maxSize")
        {
            LOG_INFO << "maxSize: " << value;
            _maxSize = atoi(value.c_str());
        }
        else if (key == "maxIdleTime")
        {
            LOG_INFO << "maxIdleTime: " << value;
            _maxIdleTime = atoi(value.c_str());
        }
        else if (key == "connectionTimeOut")
        {
            LOG_INFO << "connectionTimeOut: " << value;
            _connectionTimeOut = atoi(value.c_str());
        }
    }
    
    fclose(pf);  // 重要：关闭文件
    return true;
}

// 初始化连接池
ConnectionPool::ConnectionPool()
{
    // 加载配置项
    if (!loadConfigFile())
    {
        LOG_ERROR << "load config file fail!";
        return;
    }
    
    // 创建初始数量的连接
    for (int i = 0; i < _initSize; ++i)
    {
        Connection* p = new Connection();
        if (p->connect(_ip, _port, _username, _password, _dbname))
        {
            // 链接数据库
            p->refreshAliveTime();
            _connectionQue.push(p);
            _connectionCnt++;
        }
        else
        {
            delete p;
            LOG_ERROR << "create initial connection fail!";
        }
    }
    
    // 启动一个新的线程，作为连接的生产者
    thread produce(bind(&ConnectionPool::produceConnectionTask, this));
    produce.detach();
    
    // 启动一个新的定时线程，扫描超过maxIdleTime时间的空闲连接，进行对应的连接回收
    thread scanner(bind(&ConnectionPool::scanConnectionTask, this));
    scanner.detach();
}

void ConnectionPool::produceConnectionTask()
{
    for (;;)
    {
        unique_lock<mutex> lock(_queueMutex);
        while (!_connectionQue.empty())
        {
            cv.wait(lock); // 队列不空，此处生产线程进入等待状态
        }
        
        // 连接数量没有到达上限，继续创建新的连接
        if (_connectionCnt < _maxSize)
        {
            Connection* p = new Connection();
            if (p->connect(_ip, _port, _username, _password, _dbname))
            {
                p->refreshAliveTime();
                _connectionQue.push(p);
                _connectionCnt++;
                LOG_INFO << "create new connection, current pool size: " << _connectionCnt;
            }
            else
            {
                delete p;
                LOG_ERROR << "create connection fail in produce task!";
            }
        }
        
        // 通知消费者线程，可以消费连接了
        cv.notify_all();
    }
}

void ConnectionPool::scanConnectionTask()
{
    for (;;)
    {
        // 通过sleep模拟定时效果 池最大空闲时间
        this_thread::sleep_for(chrono::seconds(_maxIdleTime));
        
        // 扫描整个队列，释放多余的连接
        unique_lock<mutex> lock(_queueMutex);
        while (_connectionCnt > _initSize)
        {
            Connection* p = _connectionQue.front();
            //auto idleDuration = curTime - connTime; 空闲时长
            //auto idleSeconds = duration_cast<seconds>(idleDuration).count();
            if (duration_cast<seconds>(steady_clock::now() - p->getAliveTime()).count() >= _maxIdleTime)
            {
                _connectionQue.pop();
                _connectionCnt--;
                delete p; // 调用~Connection()释放连接
                LOG_INFO << "remove timeout connection, current pool size: " << _connectionCnt;
            }
            else
            {
                break; // 队头的连接没有超时，其它连接肯定没有超时
            }
        }
    }
}
// 获取链接接口
shared_ptr<Connection> ConnectionPool::getConnection()
{
    unique_lock<mutex> lock(_queueMutex);
    while (_connectionQue.empty())
    {
        // 如果为空，需要等待
        if (cv_status::timeout == cv.wait_for(lock, chrono::milliseconds(_connectionTimeOut)))
        {
            if (_connectionQue.empty())
            {
                LOG_ERROR << "get connection timeout!";
                return nullptr;
            }
        }
    }
    
    /*
    shared_ptr智能指针析构时，会把connection资源直接delete掉，相当于
    调用connection的析构函数，connection就被close掉了。
    自定义shared_ptr的释放资源的方式，把connection直接归还到queue当中
    */
    shared_ptr<Connection> sp(_connectionQue.front(), 
        [&](Connection* pcon) {
            // 这里是在服务器应用线程中调用的，所以一定要考虑队列的线程安全操作
            unique_lock<mutex> lock(_queueMutex);
            pcon->refreshAliveTime(); // 刷新一下开始空闲的起始时间
            _connectionQue.push(pcon);
        });
    
    _connectionQue.pop();
    cv.notify_all(); // 消费完连接以后，通知生产者线程检查一下，如果队列为空了，赶紧生产连接
    
    return sp;
}

// ConnectionRAII类实现
ConnectionRAII::ConnectionRAII(ConnectionPool* pool)
    : _pool(pool)
{
    _conn = pool->getConnection();
    if (!_conn)
    {
        LOG_ERROR << "Failed to get connection from pool";
    }
}

ConnectionRAII::~ConnectionRAII()
{
    // 连接会在shared_ptr析构时自动归还到连接池
}