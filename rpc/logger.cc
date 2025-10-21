#include "logger.h"

// 获取日志接口
Logger& Logger::getLOGInstance()
{
    static Logger logger;
    return logger;
}

Logger::Logger()
{
    //启动写日志线程
    std::thread writrLogTask([&](){
        for(;;)
        {
            time_t now = time(nullptr);
            tm *nowtm = localtime(&now);

            char fileName[128];
            sprintf(fileName, "%d-%d-%d-log.txt", nowtm->tm_year+1900, nowtm->tm_mon, nowtm->tm_mday);

            FILE *pf = fopen(fileName, "a");
            if(pf == nullptr)
            {
                std::cout<< "logger file :" << fileName << " open error!" << std::endl;
                exit(EXIT_FAILURE);
            }
            //读日志队列写入LOG
            std::pair<LogLevel, std::string> msg = _lckQue.Pop();
            std::string LogMsg = msg.second;
            std::string Level;
            switch (msg.first)
            {
            case INFO:
                LogMsg.insert(0, "[INFO]");
                break;
            case ERROR:
                LogMsg.insert(0, "[ERROR]");
            default:
                break;
            }
            //添加时间
            char timeMsg[128] = {0};
            sprintf(timeMsg, "%d:%d:%d =>", nowtm->tm_hour, nowtm->tm_min, nowtm->tm_sec);
            LogMsg.insert(0, timeMsg);
            LogMsg.append("\n");
            fputs(LogMsg.c_str(), pf);
            fclose(pf);
        }
    });
    writrLogTask.detach();
}

// 写日志
void Logger::LOG(LogLevel level, std::string msg)
{
    _lckQue.Push(std::make_pair(level, msg));
}
