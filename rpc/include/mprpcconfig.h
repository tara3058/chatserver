#pragma once

#include <unordered_map>
#include <string>

class MprpcConfig
{
private:
    // 配置信息
    std::unordered_map<std::string, std::string> _Configmap;

public:
    //加载配置信息文件
    void LoadConfigFile(const char *config_file);
    //查询配置项信息
    std::string LoadConfig(const std::string &key);
    //去掉字符串前后的空格
    void Trim(std::string &buf);
};

