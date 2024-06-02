/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 18:29:45
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 20:42:36
 * @FilePath: /liveServer/src/base/LogStream.cpp
 * @Description:  learn 
 */
#include "LogStream.h"
#include "TTime.h"
#include <sys/syscall.h>
#include <cstring>

using namespace tmms::base;
Logger *tmms::base::g_logger;

static thread_local pid_t thread_id = 0;
const char *log_string[] = {
    " TRACE ",
    " DEBUG ",
    " INFO ",
    " WARN ",
    " ERROR "
};

LogStream::LogStream(Logger *logger, const char *file, int line, LogLevel logLevel, const char *func)
:logger_(logger)
{
    // 用指针避免拷贝
    // c库查找函数strrchr
    const char* file_name = strrchr(file, '/');
    if(file_name)
    {
        file_name = file_name + 1;      // /后面的就是文件名
    }
    else
    {
        file_name = file;   
    }

    stream_ << TTime::ISOTime() << " ";

    if(thread_id == 0)
    {
        thread_id = static_cast<pid_t>(::syscall(SYS_gettid));
    }
    
    stream_ << thread_id;
    stream_ << log_string[logLevel];
    stream_ << " [" << file_name << ":" << line << "] ";
    if(func)
    {
        stream_ << " [" << func << "] ";
    }
}

LogStream::~LogStream()
{
    stream_ << "\n";
    // 每次输入日志都会调用写入
    logger_->Write(stream_.str());
}
