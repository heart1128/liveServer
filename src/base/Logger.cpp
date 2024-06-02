/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 18:20:04
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-02 14:37:07
 * @FilePath: /liveServer/src/base/Logger.cpp
 * @Description:  learn 
 */
#include "Logger.h"
#include <iostream>

using namespace tmms::base;

Logger::Logger(FileLog::ptr &log)
:log_(log)
{
}

/// @brief 设置日志等级
/// @param level LogLevel枚举变量
void Logger::SetLogLevel(const LogLevel &level)
{
    level_ = level;
}

/// @brief 获取日志等级
LogLevel Logger::GetLogLevel() const
{
    return level_;
}

/// @brief 写入日志文件
/// @param msg 
void Logger::Write(const std::string &msg)
{
    if(log_)
    {
        log_->WriteLog(msg);
    }
    else
    {
        std::cout << msg << std::endl;
    }
}
