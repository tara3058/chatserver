#include "mprpcconsumer.h"
#include <string>
#include "mprpcheader.pb.h"
#include"mprpcapplication.h"

#include <sys/types.h>
#include<sys/socket.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<errno.h>
#include <unistd.h>

#include"zookeeperutil.h"

// 设置超时
void Mprpcchannel::Setimeout(int sockfd, int timeoutSec){
    struct timeval timeout;
    timeout.tv_sec = timeoutSec;
    timeout.tv_usec = 0;

    //发送超时
    setsockopt(sockfd,SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
    //接收超时
    setsockopt(sockfd,SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

// 设置服务端地址
bool Mprpcchannel::GetSeverAddr(const std::string& serviceName, const std::string& methodName, std::string& ip, uint16_t& port){
    
    //zk
    ZkClient zkcli;
    zkcli.Start();
    // /service_name/method_name
    std::string methodPath = "/" + serviceName + "/" + methodName;
    // data: ip:rpc_port
    std::string hostData = zkcli.GetData(methodPath.c_str());
    if(hostData == "")
    {
        LOG_ERROR("method is not exist");
        return false;
    }
    int idx = hostData.find(":");
    if(-1 == idx)
    {
        LOG_ERROR(" address is invalid!");
        return false;
    }
    ip = hostData.substr(0, idx);
    port = atoi(hostData.substr(idx+1, hostData.size()-idx).c_str());
    return true;
}

void Mprpcchannel::CallMethod(const google::protobuf::MethodDescriptor *method,
                              google::protobuf::RpcController *controller,
                              const google::protobuf::Message *request,
                              google::protobuf::Message *response,
                              google::protobuf::Closure *done)
{
    const google::protobuf::ServiceDescriptor *SerDsc = method->service();
    std::string ServName = SerDsc->name();
    std::string MethName = method->name();

    // 获取参数的序列化字符串长度 args_size
    uint32_t args_size = 0;
    std::string argsStr;
    //arg
    if (request->SerializeToString(&argsStr))
    {
        args_size = argsStr.size();
    }
    else
    {
        LOG_ERROR("serialize request_str error!");
        controller->SetFailed("serialize request_str error!");
        return;
    }

    //header
    mprpc::mpRpcHeader mprpcHeader;
    mprpcHeader.set_service_name(ServName);
    mprpcHeader.set_method_name(MethName);
    mprpcHeader.set_args_size(args_size);
    //headerSize
    uint32_t headerSize = 0;
    std::string mprpcHeaderStr;
    if(mprpcHeader.SerializePartialToString(&mprpcHeaderStr))
    {
        headerSize = mprpcHeaderStr.size();
    }
    else
    {
        LOG_ERROR("serialize mprpcHeader error!");
        controller->SetFailed("serialize mprpcHeader error!");
        //std::cout << "serialize mprpcHeader error!" << std::endl;
        return;
    }

    //组合字符流
    //（headerSize + header(service/method/argsize) + argStr）
    std::string sendBuf;
    sendBuf.insert(0, std::string((char*)&headerSize, 4));
    sendBuf += mprpcHeaderStr;
    sendBuf += argsStr;

    // 打印调试信息
    std::cout << "============================================" << std::endl;
    std::cout << "header_size: " << headerSize << std::endl; 
    std::cout << "rpc_header_str: " << mprpcHeaderStr << std::endl; 
    std::cout << "service_name: " << ServName << std::endl; 
    std::cout << "method_name: " << MethName << std::endl; 
    std::cout << "args_str: " << argsStr << std::endl; 
    std::cout << "============================================" << std::endl;

    //在zk中获取服务地址
    std::string ip;
    uint16_t port;
    if(!GetSeverAddr(ServName, MethName, ip, port)){
        controller->SetFailed("rpc service not exist");
        return;
    }

    //tcp
    int clientfd = socket(AF_INET, SOCK_STREAM, 0);
    if( -1 == clientfd)
    {
        std::cout << "create socket error!" << std::endl;
        controller->SetFailed("create socket error");
        return;
        //exit(EXIT_FAILURE);
    }

    // 读取配置文件rpcserver的信息
    //std::string ip = MprpcApplication::GetConfig().LoadConfig("rpcserverip");
    //uint16_t port = atoi(MprpcApplication::GetConfig().LoadConfig("rpcserverport").c_str());
    // 建立连接超时
    Setimeout(clientfd, 5);
   

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr= inet_addr(ip.c_str());

    // 连接rpc服务节点
    if(-1 == connect(clientfd, (struct sockaddr*)&server_addr, sizeof(server_addr)))
    {   
        close(clientfd);
        std::cout << "connect error!" << std::endl;
        controller->SetFailed("connect error!");
        return;
    }
    // 发送rpc请求
    if(-1 == send(clientfd, sendBuf.c_str(), sendBuf.size(), 0))
    {
        close(clientfd);
        std::cout << "send error!" << std::endl;
        controller->SetFailed("send error!");
        return;
    }

    // 接收rpc请求的响应值
    char recvBuf[1024]={0};
    int recvSize = 0;
    if(-1 == (recvSize = recv(clientfd, recvBuf, 1024, 0)))
    {
        close(clientfd);
        std::cout << "receive error!" << std::endl;
        return;
    }

    //// 反序列化rpc调用的响应数据
    if(!response->ParseFromArray(recvBuf, recvSize))
    {
        close(clientfd);
        std::cout << "parse error!" << std::endl;
        return;
    }

    close(clientfd);
}