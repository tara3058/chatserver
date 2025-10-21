#include "mprpcprovider.h"
#include "mprpcapplication.h"

// 开启节点 提供RPC服务
void MprpcProvider::StartMprpc()
{
    // 读取配置文件rpcserver的信息
    std::string ip = MprpcApplication::GetConfig().LoadConfig("rpcserverip");
    uint16_t port = atoi(MprpcApplication::GetConfig().LoadConfig("rpcserverport").c_str());
    muduo::net::InetAddress addr(ip, port);
    // 创建TcpServer对象
    muduo::net::TcpServer server(&_event_loop, addr, "RpcProvider");
    // 绑定连接回调和消息读写回调方法
    server.setConnectionCallback(std::bind(&MprpcProvider::OnConnection, this, std::placeholders::_1));
    server.setMessageCallback(std::bind(&MprpcProvider::OnMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    // 设置muduo库的线程数量
    server.setThreadNum(4);

    // 启动网络服务
    server.start();


    //注册当前rpc节点到zk
    ZkClient zkcli;
    zkcli.Start();

     // service_name为永久性节点    method_name为临时性节点
    for (auto &sp : _serviceInfo)
    {
        std::string servicePath = "/" + sp.first;
        LOG_INFO("service_name:%s, method_name:%s", servicePath.c_str(), sp.first.c_str());
        zkcli.Create(servicePath.c_str(), nullptr, 0);
        for(auto &mp : sp.second._methodInfoMap)
        {
            // /service_name/method_name
            std::string methodPath = servicePath + "/" + mp.first;
            char methodPath_data[128] ={0};
            // data—》rpc_ip:rpc_port
            sprintf(methodPath_data, "%s:%d", ip.c_str(), port);
            zkcli.Create(methodPath.c_str(), methodPath_data, strlen(methodPath_data), ZOO_EPHEMERAL);
        }
    }


    _event_loop.loop();
}

// 连接回调
void MprpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
    if (!conn->connected())
    {
        // 断开和 rpc client连接
        conn->shutdown();
    }
}

// 读写回调 （headerSize + header + arg）
void MprpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
                              muduo::net::Buffer *buffer,
                              muduo::Timestamp)
{
    // 接受RPC请求的字符流
    std::string recvBuf = buffer->retrieveAllAsString();

    //检查数据长度
    if(recvBuf.size() < 4){
        LOG_ERROR("recvBuf size is too small");
        return;
    }

    // headerSize
    //知服务和方法名字符长度
    uint32_t headerSize = 0;
    recvBuf.copy((char *)&headerSize, 4, 0);

    // header
    std::string mprpc_headerStr = recvBuf.substr(4, headerSize);
    std::string ServName;
    std::string MethName;
    uint32_t args_size;

    if(!Paresrpcheader(mprpc_headerStr, ServName,MethName, args_size)){
        LOG_ERROR("Paresrpcheader error");
        return;
    }
    
    //arg
    std::string argsStr = recvBuf.substr(4 + headerSize, args_size);

    // 打印调试信息
    LOG_INFO("============================================");
    LOG_INFO("header_size: %d", headerSize);
    LOG_INFO("rpc_header_str: %s", mprpc_headerStr.c_str());
    LOG_INFO("service_name: %s", ServName.c_str());
    LOG_INFO("method_name: %s", MethName.c_str());
    LOG_INFO("args_str: %s", argsStr.c_str());
    LOG_INFO("============================================");

    // 查找请求服务及对象
    auto it = _serviceInfo.find(ServName);
    if (it == _serviceInfo.end())
    {   
        LOG_ERROR("%s is not exist!", ServName.c_str());
        return;
    }
    auto methit = it->second._methodInfoMap.find(MethName);
    if (methit == it->second._methodInfoMap.end())
    {
        LOG_ERROR("%s is not exist!", MethName.c_str());
        return;
    }
    google::protobuf::Service *service = it->second._service;
    const google::protobuf::MethodDescriptor *method = methit->second;

    // 生成request请求
    google::protobuf::Message *request = service->GetRequestPrototype(method).New();
    

    //获取方法参数的字符流数据
    if (!request->ParseFromString(argsStr))
    {
        LOG_ERROR("request parse error, content: %s", argsStr.c_str());
        return;
    }

    //生成response请求
    google::protobuf::Message *response = service->GetResponsePrototype(method).New();

    //给method的调用绑定一个回调
    //template <typename Class, typename Arg1, typename Arg2>
    google::protobuf::Closure *done = google::protobuf::NewCallback<MprpcProvider, 
                                                                    const muduo::net::TcpConnectionPtr&,
                                                                    google::protobuf::Message*>
                                                                    (this, 
                                                                    &MprpcProvider::SendmprpcResponse,
                                                                    conn,
                                                                    response);

    //根据远端请求字符流，将请求分配到该节点上发布的相应方法
    service->CallMethod(method, nullptr, request, response, done);

}

// Closure的回调操作，用于序列化rpc的响应和网络发送
void MprpcProvider::SendmprpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
    std::string responseStr;
    // response进行序列化
    if(response->SerializePartialToString(&responseStr))
    {
        //序列化成功后 通过框架网络返回
        conn->send(responseStr);
    }
    else
    {
        LOG_ERROR("serialize response_str error!" );
    }
    //清理资源
    delete response;
    conn->shutdown();
    
}


// 发布服务接口
void MprpcProvider::NotifyService(google::protobuf::Service *service)
{

    // 获取了服务对象的信息
    const google::protobuf::ServiceDescriptor *pSerDsc = service->GetDescriptor();
    // 服务名字
    std::string SerName = pSerDsc->name();
    // 服务包含的方法数量
    int MethodCnt = pSerDsc->method_count();

    //打印日志
    LOG_INFO("service_name:%s", SerName.c_str());
 
    MethodStruct method_struct;
    method_struct._service = service;
    for (int i = 0; i < MethodCnt; ++i)
    {
        // 获取服务中的方法
        const google::protobuf::MethodDescriptor *pMethDsc = pSerDsc->method(i);
        std::string MethName = pMethDsc->name();
        // 储存方法
        method_struct._methodInfoMap.insert({MethName, pMethDsc});

        //打印日志
        LOG_INFO("method_name:%s", MethName.c_str());

    }
    // 存储服务
    _serviceInfo.insert({SerName, method_struct});
}

//解析header头
bool MprpcProvider::Paresrpcheader(const std::string& headerstr, std::string& serviceName, std::string& methodName, uint32_t& argsSize){
    mprpc::mpRpcHeader mprpcHeader;
    if (!mprpcHeader.ParseFromString(headerstr))
    {
        // 反序列化失败
        LOG_ERROR("rpc_header_str parse error!");
        return false;
    }

    // 反序列化成功
    serviceName = mprpcHeader.service_name();
    methodName = mprpcHeader.method_name();
    argsSize = mprpcHeader.args_size();
    return true;

    
}
