#ifndef DB_H
#define DB_H

// 已被ConnectionPool替代
// 请使用ConnectionPool::getConnection()获取数据库连接
// 仅为保持向后兼容性而保留

#include <mysql/mysql.h>
#include <string>

using namespace std;

/**
 * @see ConnectionPool
 * MySQL单连接封装类
 */
class MySQL
{
private:
    MYSQL *_conn;
public:
    //初始化
    MySQL();
    //释放
    ~MySQL();
    // 连接数据库
    bool connect();
    // 更新操作
    bool update(string sql);
    // 查询操作
    MYSQL_RES *query(string sql);
    // 获取连接
    MYSQL* getSQLConnection();
};

#endif