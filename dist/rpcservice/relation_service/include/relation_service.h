// dist/rpcservice/relation_service/include/relation_service.h
#pragma once

#include "servicebase.h"
#include "relation.pb.h"
#include "friendmodel.hpp"
#include "groupmodel.hpp"
#include "usermodel.hpp"
#include "connectionpool.h"
#include <muduo/base/Logging.h>

// 关系服务实现
class RelationServiceImpl : public ServiceBase, public relationservice::RelationService
{
public:
    RelationServiceImpl(const std::string& ip, uint16_t port);
    
protected:
    // 实现服务基类的虚函数
    void InitRpcService() override;
    void InitDatabasePool() override;
    
    // 实现RelationService的RPC方法
    void AddFriend(::google::protobuf::RpcController* controller,
                   const ::relationservice::AddFriendRequest* request,
                   ::relationservice::AddFriendResponse* response,
                   ::google::protobuf::Closure* done) override;
                            
    void RemoveFriend(::google::protobuf::RpcController* controller,
                      const ::relationservice::RemoveFriendRequest* request,
                      ::relationservice::RemoveFriendResponse* response,
                      ::google::protobuf::Closure* done) override;
                         
    void CreateGroup(::google::protobuf::RpcController* controller,
                     const ::relationservice::CreateGroupRequest* request,
                     ::relationservice::CreateGroupResponse* response,
                     ::google::protobuf::Closure* done) override;
                            
    void JoinGroup(::google::protobuf::RpcController* controller,
                   const ::relationservice::JoinGroupRequest* request,
                   ::relationservice::JoinGroupResponse* response,
                   ::google::protobuf::Closure* done) override;
                           
    void LeaveGroup(::google::protobuf::RpcController* controller,
                    const ::relationservice::LeaveGroupRequest* request,
                    ::relationservice::LeaveGroupResponse* response,
                    ::google::protobuf::Closure* done) override;
                            
    void GetFriends(::google::protobuf::RpcController* controller,
                    const ::relationservice::GetFriendsRequest* request,
                    ::relationservice::GetFriendsResponse* response,
                    ::google::protobuf::Closure* done) override;
                            
    void GetGroups(::google::protobuf::RpcController* controller,
                   const ::relationservice::GetGroupsRequest* request,
                   ::relationservice::GetGroupsResponse* response,
                   ::google::protobuf::Closure* done) override;

private:
    // 好友模型
    FriendModel _friendModel;
    
    // 群组模型
    GroupModel _groupModel;
    
    // 用户模型
    UserModel _userModel;
    
    // 数据库连接池
    ConnectionPool* _connectionPool;
};