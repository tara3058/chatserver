#pragma once
#include"mprpcconfig.h"
#include"mprpccontroller.h"
#include"mprpcconsumer.h"
#include"mprpcprovider.h"
#include"logger.h"


//mprpc框架基础类
class MprpcApplication
{
public:
    //框架初始化
    static void Init(int argc, char **argv);
    //框架接口
    static MprpcApplication& GetInstance();
    //配置文件接口
    static MprpcConfig& GetConfig();

private:
    MprpcApplication(){}
    MprpcApplication(const MprpcApplication&) = delete;
    MprpcApplication(MprpcApplication&&) = delete;

    //配置文件
    static MprpcConfig _config; 

};
