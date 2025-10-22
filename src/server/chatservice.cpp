#include "chatservice.hpp"
#include "public.hpp"
#include <muduo/base/Logging.h>

using namespace muduo;
using namespace placeholders;
using namespace std;

// 单例对象的接口
Chatservice *Chatservice::instance()
{
    static Chatservice service;
    return &service;
}

// 注册业务和对应的回调
Chatservice::Chatservice()
{
    _msgHandlerMap.insert({LOGIN_MSG, std::bind(&Chatservice::login, this, _1, _2, _3)});
    _msgHandlerMap.insert({REG_MSG, std::bind(&Chatservice::reg, this, _1, _2, _3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG, std::bind(&Chatservice::oneChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_FRIEND_ACK, std::bind(&Chatservice::addFriend, this, _1, _2, _3)});
    _msgHandlerMap.insert({CREATE_GROUP_ACK, std::bind(&Chatservice::createGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({ADD_GROUP_ACK, std::bind(&Chatservice::addGroup, this, _1, _2, _3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG, std::bind(&Chatservice::groupChat, this, _1, _2, _3)});
    _msgHandlerMap.insert({LOGINOUT_MSG, std::bind(&Chatservice::loginout, this, _1, _2, _3)});

    // 链接redis
    if (_redis.connect())
    {
        // 绑定回调
        _redis.init_notify_handler(std::bind(&Chatservice::handlerRedisSubscirbMsg, this, _1, _2));
    }
}

// 登录业务
void Chatservice::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"].get<int>();
    string pwd = js["password"];

    User user = _userModel.dbquery(id);
    if (user.getId() == id && user.getPwd() == pwd)
    {
        if (user.getState() == "online")
        {
            // 该用户已经登录，不允许重复登录
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;
            response["errmsg"] = "this account is using, input another!";
            conn->send(response.dump());
        }
        else
        {
            // 登录成功，记录用户连接信息
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id, conn});
            }
            // 登录成功，更新用户状态信息 state offline=>online
            user.setState("online");
            _userModel.updateState(user);

            // id用户登录成功后，向redis订阅channel(id)
            _redis.subscribe(id);

            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;
            response["id"] = user.getId();
            response["name"] = user.getName();

            // 查询该用户是否有离线消息
            vector<string> vec = _offlineMsgModel.query(id);
            if (!vec.empty())
            {
                response["offlinemsg"] = vec;
                // 读取该用户的离线消息后，把该用户的所有离线消息删除掉
                _offlineMsgModel.remove(id);
            }

            // 查询该用户的好友信息并返回
            vector<User> friendvec = _friendModel.query(id);
            if (!friendvec.empty())
            {
                vector<string> vec2;
                for (User &user : friendvec)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }

            // 查询用户的群组信息
            vector<Group> groupuserVec = _groupModel.queryGroups(id);
            if (!groupuserVec.empty())
            {
                vector<string> groupvec;
                for (Group &group : groupuserVec)
                {
                    json grpjs;
                    grpjs["groupid"] = group.getId();
                    grpjs["groupname"] = group.getName();
                    grpjs["groupdesc"] = group.getDesc();
                    vector<string> uservec;
                    for (const GroupUser &groupuser : group.getGroupUsers())
                    {
                        json js;
                        js["id"] = groupuser.getId();
                        js["name"] = groupuser.getName();
                        js["state"] = groupuser.getState();
                        js["role"] = groupuser.getRole();
                        uservec.push_back(js.dump());
                    }
                    grpjs["groupuser"] = uservec;
                    groupvec.push_back(grpjs.dump());
                }
                response["groups"] = groupvec;
            }

            conn->send(response.dump());
        }
    }
    else
    {
        // 该用户不存在，用户存在但是密码错误，登录失败
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;
        response["errmsg"] = "id or password is invalid!";
        conn->send(response.dump());
    }
}
// 注册业务
void Chatservice::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    string name = js["name"];
    string pwd = js["password"];

    User user;
    user.setName(name);
    user.setPwd(pwd);
    bool state = _userModel.insert(user);
    if (state)
    {
        // 注册成功
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;
        response["id"] = user.getId();
        conn->send(response.dump());
    }
    else
    {
        // 注册失败
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 处理注销业务
void Chatservice::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if (it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    // 取消redis订阅
    _redis.unsubscribe(userid);

    // 更新用户拽他
    User user(userid, "", "", "offline");
    _userModel.updateState(user);
}

// 一对一聊天业务
void Chatservice::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["toid"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if (it != _userConnMap.end())
        {
            // toid在线，转发消息   推回服务器
            it->second->send(js.dump());
            return;
        }
    }

    // 查询是否其他服务器
    User user = _userModel.dbquery(toid);
    if (user.getState() == "online")
    {
        _redis.publish(toid, js.dump());
    }
    else
    {
        // toid不在线，存储离线消息
        _offlineMsgModel.insert(toid, js.dump());
    }
}

// 添加好友业务 msgid id friendid
void Chatservice::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int friendid = js["friendid"].get<int>();

    bool state = _friendModel.insert(userid, friendid);
    if (state)
    {
        // 添加成功
        json response;
        response["msgid"] = ADD_FRIEND_ACK;
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 添加失败
        json response;
        response["msgid"] = ADD_FRIEND_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 创建群组业务
void Chatservice::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string name = js["groupname"];
    string desc = js["groupdesc"];

    // 存储新创建的群组信息
    Group group(-1, name, desc);

    if (_groupModel.createGroup(group))
    {
        // 成功
        //  存储群组创建人信息
        _groupModel.addGroup(userid, group.getId(), "creator");
        json response;
        response["msgid"] = CREATE_GROUP_ACK;
        response["grouid"] = group.getId();
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 失败
        json response;
        response["msgid"] = CREATE_GROUP_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 加入群组业务
void Chatservice::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();

    bool state = _groupModel.addGroup(userid, groupid, "normal");
    if (state)
    {
        // 添加成功
        json response;
        response["msgid"] = ADD_GROUP_ACK;
        response["errno"] = 0;
        conn->send(response.dump());
    }
    else
    {
        // 添加失败
        json response;
        response["msgid"] = ADD_GROUP_ACK;
        response["errno"] = 1;
        conn->send(response.dump());
    }
}

// 群组聊天业务
void Chatservice::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = _groupModel.queryGroupUsers(userid, groupid);

    {
        lock_guard<mutex> lock(_connMutex);
        for (int id : useridVec)
        {
            auto it = _userConnMap.find(id);
            if (it != _userConnMap.end())
            {
                // 转发群消息
                it->second->send(js.dump());
            }
            else
            {
                // 查询是否其他服务器
                User user = _userModel.dbquery(id);
                if (user.getState() == "online")
                {
                    _redis.publish(id, js.dump());
                }
                else
                {
                    //// 存储离线群消息
                    _offlineMsgModel.insert(id, js.dump());
                }
            }
        }
    }
}

// 客户端异常业务
void Chatservice::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for (auto it = _userConnMap.begin(); it != _userConnMap.end(); it++)
        {
            if (it->second == conn)
            {
                // 从map表删除用户的链接信息
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }

    // 取消redis订阅
    _redis.unsubscribe(user.getId());

    if (user.getId() != -1) // 为有效用户
    {
        user.setState("offline");
        _userModel.updateState(user);
    }
}

// 服务器异常，业务重置方法
void Chatservice::reset()
{
    // 把online状态的用户，设置成offline
    _userModel.resetState();
}

// 获取业务对应的处理器
MsgHandler Chatservice::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if (it == _msgHandlerMap.end())
    {
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp)
        {
            LOG_ERROR << "msgid" << "can't find handler";
        };
    }
    else
    {
        return _msgHandlerMap[msgid];
    }
}

// redis处理器
void Chatservice::handlerRedisSubscirbMsg(int userid, string msg)
{
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(userid);
    if (it != _userConnMap.end())
    {
        // toid在线，转发消息   推回服务器
        it->second->send(msg);
        return;
    }
    // 存储该用户的离线消息
    _offlineMsgModel.insert(userid, msg);
}