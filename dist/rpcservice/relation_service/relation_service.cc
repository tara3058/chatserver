#include "include/relation_service.h"
#include "mprpcprovider.h"
#include "logger.h"
#include <iostream>

RelationServiceImpl::RelationServiceImpl(const std::string& ip, uint16_t port)
    : ServiceBase("RelationService", ip, port), _connectionPool(nullptr)
{
    std::cout << "RelationServiceImpl created" << std::endl;
}

// 实现服务基类的虚函数
void RelationServiceImpl::InitRpcService(){
    MprpcProvider provider;
    provider.NotifyService(this);
    std::cout << "RelationService RPC service initialized" << std::endl;
}

void RelationServiceImpl::InitDatabasePool(){
    // 初始化数据库连接池
    _connectionPool = ConnectionPool::getConnectionPool();
    std::cout << "RelationService database pool initialized" << std::endl;
}

// 实现RelationService的RPC方法
void RelationServiceImpl::AddFriend(::google::protobuf::RpcController* controller,
                   const ::relationservice::AddFriendRequest* request,
                   ::relationservice::AddFriendResponse* response,
                   ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::AddFriend called" << std::endl;
    
    // 添加好友关系
    bool result = _friendModel.insert(request->user_id(), request->friend_id());
    
    if (result) {
        response->set_error_code(0);
        response->set_error_msg("Success");
    } else {
        response->set_error_code(1);
        response->set_error_msg("Failed to add friend");
    }
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::RemoveFriend(::google::protobuf::RpcController* controller,
                      const ::relationservice::RemoveFriendRequest* request,
                      ::relationservice::RemoveFriendResponse* response,
                      ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::RemoveFriend called" << std::endl;
    // TODO: 实现删除好友的逻辑
    
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::CreateGroup(::google::protobuf::RpcController* controller,
                     const ::relationservice::CreateGroupRequest* request,
                     ::relationservice::CreateGroupResponse* response,
                     ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::CreateGroup called" << std::endl;
    
    // 创建群组
    Group group(-1, request->group_name(), request->group_desc());
    bool result = _groupModel.createGroup(group);
    
    if (result) {
        response->set_error_code(0);
        response->set_error_msg("Success");
        // TODO: 获取创建的群组ID并设置到响应中
    } else {
        response->set_error_code(1);
        response->set_error_msg("Failed to create group");
    }
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::JoinGroup(::google::protobuf::RpcController* controller,
                   const ::relationservice::JoinGroupRequest* request,
                   ::relationservice::JoinGroupResponse* response,
                   ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::JoinGroup called" << std::endl;
    
    // 加入群组
    bool result = _groupModel.addGroup(request->user_id(), request->group_id(), "normal");
    
    if (result) {
        response->set_error_code(0);
        response->set_error_msg("Success");
    } else {
        response->set_error_code(1);
        response->set_error_msg("Failed to join group");
    }
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::LeaveGroup(::google::protobuf::RpcController* controller,
                    const ::relationservice::LeaveGroupRequest* request,
                    ::relationservice::LeaveGroupResponse* response,
                    ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::LeaveGroup called" << std::endl;
    // TODO: 实现退出群组的逻辑
    
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::GetFriends(::google::protobuf::RpcController* controller,
                    const ::relationservice::GetFriendsRequest* request,
                    ::relationservice::GetFriendsResponse* response,
                    ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::GetFriends called" << std::endl;
    
    // 获取好友列表
    vector<User> friends = _friendModel.query(request->user_id());
    
    // 添加到响应中
    for (const auto& friendUser : friends) {
        relationservice::FriendInfo* friendInfo = response->add_friends();
        friendInfo->set_id(friendUser.getId());
        friendInfo->set_name(friendUser.getName());
        friendInfo->set_state(friendUser.getState());
    }
    
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}
                        
void RelationServiceImpl::GetGroups(::google::protobuf::RpcController* controller,
                   const ::relationservice::GetGroupsRequest* request,
                   ::relationservice::GetGroupsResponse* response,
                   ::google::protobuf::Closure* done)
{
    std::cout << "RelationService::GetGroups called" << std::endl;
    
    // 获取群组列表
    vector<Group> groups = _groupModel.queryGroups(request->user_id());
    
    // 添加到响应中
    for (const auto& group : groups) {
        relationservice::GroupInfo* groupInfo = response->add_groups();
        groupInfo->set_id(group.getId());
        groupInfo->set_name(group.getName());
        groupInfo->set_desc(group.getDesc());
        
        // 添加群组用户信息
        for (const auto& groupUser : group.getGroupUsers()) {
            relationservice::GroupUserInfo* groupUserInfo = groupInfo->add_users();
            groupUserInfo->set_id(groupUser.getId());
            groupUserInfo->set_name(groupUser.getName());
            groupUserInfo->set_state(groupUser.getState());
            groupUserInfo->set_role(groupUser.getRole());
        }
    }
    
    response->set_error_code(0);
    response->set_error_msg("Success");
    
    // 完成RPC调用
    done->Run();
}