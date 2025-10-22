#ifndef GROUP_H
#define GROUP_H

#include "groupuser.hpp"
#include<vector>
#include<string>
using namespace std;

class Group
{
public:
    //组参数
    Group(int id = -1, string name = "", string desc = "")
    {
        this->id = id;
        this->name = name;
        this->desc = desc;
    }

    //设置组信息参数
    void setId(int id) { this->id = id; }
    void setName(string name) { this->name = name; }
    void setDesc(string desc) { this->desc = desc; }
    //获取组信息参数
    int getId() const { return this->id; }
    string getName() const { return this->name; }
    string getDesc() const { return this->desc; }
    const vector<GroupUser>& getGroupUsers() const { return this->groupusers;}
    vector<GroupUser>& getGroupUsers() { return this->groupusers;}  // 非const版本

private:
    int id;
    string name;
    string desc;
    vector<GroupUser> groupusers;
};

#endif