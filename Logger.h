#pragma once 

#include <string>
#include "noncopyable.h"

enum LoggerLevel{
    INFO,//普通消息
    ERROR,//错误消息
    FATAL,//内核错误
    DEBUG//调试消息
};
//LOG_INFO
#define LOG_INFO(logmsgFormat,...)\
    do\
    {\
        Logger& logger=Logger::instance();\
        logger.setLogLevel(INFO);\
        char buf[1024]={0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)\

//LOG_ERROR
#define LOG_ERROR(logmsgFormat,...)\
    do\
    {\
        Logger& logger=Logger::instance();\
        logger.setLogLevel(ERROR);\
        char buf[1024]={0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)\
//LOG_FATAL
#define LOG_FATAL(logmsgFormat,...)\
    do\
    {\
        Logger& logger=Logger::instance();\
        logger.setLogLevel(FATAL);\
        char buf[1024]={0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
        exit(-1);\
    }while(0)\

#ifdef MUDUODEBUG
//LOG_DEBUG    
#define LOG_DEBUG(logmsgFormat,...)\
    do\
    {\
        Logger& logger=Logger::instance();\
        logger.setLogLevel(DEBUG);\
        char buf[1024]={0};\
        snprintf(buf,1024,logmsgFormat,##__VA_ARGS__);\
        logger.log(buf);\
    }while(0)\
#else
    #define LOG_DEBUG(LogmsgFormat,...)
#endif

class Logger: noncopyable{
public:
    static Logger& instance();//获取日志类实例
    void setLogLevel(int level);
    void log(std::string msg);//写日志

private:
    int LoggerLevel_;
    Logger(){};
};