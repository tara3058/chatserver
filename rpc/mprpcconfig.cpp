#include "mprpcconfig.h"
#include<iostream>

// 加载配置信息文件
void MprpcConfig::LoadConfigFile(const char *config_file)
{
    FILE *pf = fopen(config_file, "r");
    if(nullptr == pf)
    {
        std::cout << config_file << " is note exist!" << std::endl;
        exit(EXIT_FAILURE);
    }
    while(!feof(pf))
    {
        char buf[512] = {0};
        fgets(buf, 512, pf);

        // 去掉字符串前面多余的空格
        std::string readBuf(buf);
        Trim(readBuf);

        //判断注释
        if(readBuf[0] =='#' || readBuf.empty())
        {
            continue;
        }

        //解析配置项
        int idx = readBuf.find('=');
        if(idx == -1)
        {
            //不合法
            continue;
        }
        std::string key;
        std::string value;
        key = readBuf.substr(0, idx);
        //除空格
        Trim(key);
        int endix = readBuf.find('\n', idx);
        value = readBuf.substr(idx + 1, endix - 1 - idx);
        //除换行符和空格
        Trim(value);
        _Configmap.insert({key, value});




    }

}
// 查询配置项信息
std::string MprpcConfig::LoadConfig(const std::string &key)
{
    auto it = _Configmap.find(key);
    if(it == _Configmap.end())
    {
        return "";
    }
    return it->second;
}

//去掉字符串前后的空格
void MprpcConfig::Trim(std::string &buf)
{
    int idx = buf.find_first_not_of(' ');
    if(idx != -1)
    {
        //// 说明字符串前面有空格
        buf = buf.substr(idx, buf.size()-idx);
    }
    //去掉字符后面多余的空格
    idx = buf.find_last_not_of(' ');
    if(idx != -1)
    {
        // 说明字符串后面有空格
        buf = buf.substr(0, idx + 1);
    }
}