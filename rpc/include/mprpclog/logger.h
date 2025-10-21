#pragma once
#include"lockqueue.h"
#include"time.h"
#include"iostream"
#include<string>
//日志系统

// LOG_INFO("%s %d", arg1, arg2)
#define LOG_INFO(logmsgformat, ...) \
    do \
    { \
        Logger &logger = Logger::getLOGInstance(); \
        char msg[1024] ={0}; \
        snprintf(msg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.LOG(INFO, msg); \
    } while(0)

#define LOG_ERROR(logmsgformat, ...) \
    do \
    { \
        Logger &logger = Logger::getLOGInstance(); \
        char msg[1024] ={0}; \
        snprintf(msg, 1024, logmsgformat, ##__VA_ARGS__); \
        logger.LOG(ERROR, msg); \
    }while(0);


enum LogLevel
{
    INFO, //普通信息
    ERROR, //错误信息
};


class Logger
{
public:
    //获取日志接口
    static Logger& getLOGInstance();
    //设置日志级别
    //void setLogLevel(LogLevel);
    //获取日志级别
    //std::string getLogLevel();
    //写日志
    void LOG(LogLevel level, std::string msg);

private:
    int _loglevel; //日志级别
    LockQueue<std::pair<LogLevel, std::string>> _lckQue; //日志缓冲队列

    Logger();
    Logger(const Logger&) = delete;
    Logger(Logger&&) = delete;
};