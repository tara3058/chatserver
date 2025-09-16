#ifndef PUBLIC_H
#define PUBLIC_H

/*
    server和client的公共文件
*/

enum
{
    LOGIN_MSG = 1,  // 登录消息
    LOGIN_MSG_ACK,  // 登录响应消息
    REG_MSG,        // 注册消息
    REG_MSG_ACK,    // 注册响应消息
    LOGINOUT_MSG, // 注销消息
    ONE_CHAT_MSG,   // 聊天消息
    ADD_FRIEND_ACK, // 添加好友消息

    CREATE_GROUP_ACK, // 创建群组
    ADD_GROUP_ACK,    // 加入群组
    GROUP_CHAT_MSG,    // 群组聊天消息

};

#endif