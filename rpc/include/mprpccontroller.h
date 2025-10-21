#pragma once
#include<google/protobuf/service.h>
#include<string>


class MprpcController : public google::protobuf::RpcController
{
public:
    MprpcController();
    void Reset();
    bool Failed() const;
    std::string ErrorText() const;

    //未具体实现
    void StartCancel();
    void SetFailed(const std::string& reason);
    bool IsCanceled() const;
    void NotifyOnCancel(google::protobuf::Closure* callback);

private:
    bool m_failed; //RPC执行过程中的状态；
    std::string m_errText; //执行过程中的错误信息

};