#include "usermodel.hpp"
#include "connectionpool.h"
#include <iostream>

// User表的增加方法
bool UserModel::insert(User &user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "insert into user1(name, password, state) values('%s', '%s', '%s')",
            user.getName().c_str(), user.getPwd().c_str(), user.getState().c_str());

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return false;
    }
    
    if (conn->update(sql))
    {
        // 获取插入成功的用户数据生成的主键id
        user.setId(mysql_insert_id(conn->getMysqlConnection()));
        return true;
    }

    return false;
}

// 根据用户号码查询用户信息
User UserModel::dbquery(int id)
{
    // 1.组装sql语句
    char sql[1024] = {0};
    sprintf(sql, "select * from user1 where id = %d", id);

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return User();
    }
    
    MYSQL_RES *res = conn->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row != nullptr)
        {
            User user;
            user.setId(atoi(row[0]));
            user.setName(row[1]);
            user.setPwd(row[2]);
            user.setState(row[3]);
            mysql_free_result(res); // 释放指针资源
            return user;
        }
    }

    return User();
}

// 更新用户的状态信息
bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql, "update user1 set state = '%s' where id = %d", user.getState().c_str(), user.getId());

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return false;
    }
    
    if (conn->update(sql))
    {
        return true;
    }
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = "update user1 set state = 'offline' where state = 'online'";

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (conn.isValid()) {
        conn->update(sql);
    }
}