#include "groupmodel.hpp"
#include "connectionpool.h"

// 创建群组 返回id
bool GroupModel::createGroup(Group &group)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into AllGroup(groupname, groupdesc) values('%s', '%s')",
            group.getName().c_str(), group.getDesc().c_str());

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return false;
    }
    
    if (conn->update(sql))
    {
        group.setId(mysql_insert_id(conn->getMysqlConnection()));
        return true;
    }
    return false;
}

// 加入群组
bool GroupModel::addGroup(int userid, int groupid, string role)
{
    char sql[1024] = {0};
    sprintf(sql, "insert into GroupUser values(%d, %d, '%s')",
            groupid, userid, role.c_str());
    
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


// 查询用户所在群组信息  用户所在群及群内成员
vector<Group> GroupModel::queryGroups(int userid)
{
    char sql[1024] = {0};
    // 根据Groupuser信息得到AllGroup中信息
    sprintf(sql, "select a.id,a.groupname,a.groupdesc from AllGroup a inner join \
         GroupUser b on a.id = b.groupid where b.userid=%d",
            userid);

    vector<Group> groupVec;

    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return groupVec;
    }
    
    MYSQL_RES *res = conn->query(sql);
    if (res != nullptr)
    {
        // 查出userid所有的群组信息
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            Group group;
            group.setId(atoi(row[0]));
            group.setName(row[1]);
            group.setDesc(row[2]);
            groupVec.push_back(group);
        }
        mysql_free_result(res);
    }

    // 查询群组的用户信息
    for (Group &group : groupVec)
    {
        sprintf(sql, "select a.id,a.name,a.state,b.grouprole from User a \
        inner join GroupUser b on b.userid = a.id where b.groupid=%d",
                group.getId());

        MYSQL_RES *res = conn->query(sql);
        if (res != nullptr)
        {
            MYSQL_ROW row;
            while ((row = mysql_fetch_row(res)) != nullptr)
            {
                GroupUser groupuser;
                groupuser.setId(atoi(row[0]));
                groupuser.setName(row[1]);
                groupuser.setState(row[2]);
                groupuser.setRole(row[3]);
                group.getGroupUsers().push_back(groupuser);
            }
            mysql_free_result(res);
        }
    }
    return groupVec;
}

// 给用户群聊业务给群组其它成员群发消息
vector<int> GroupModel::queryGroupUsers(int userid, int groupid)
{
    char sql[1024] = {0};
    sprintf(sql, "select userid from GroupUser where groupid = %d and userid != %d", groupid, userid);

    vector<int> idVec;
    
    ConnectionPool* pool = ConnectionPool::getConnectionPool();
    ConnectionRAII conn(pool);
    
    if (!conn.isValid()) {
        return idVec;
    }
    
    MYSQL_RES *res = conn->query(sql);
    if (res != nullptr)
    {
        MYSQL_ROW row;
        while ((row = mysql_fetch_row(res)) != nullptr)
        {
            idVec.push_back(atoi(row[0]));
        }
        mysql_free_result(res);
    }
    return idVec;
}