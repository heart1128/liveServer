/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 18:16:11
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-02 14:36:55
 * @FilePath: /liveServer/src/base/Logger.h
 * @Description:  learn 
 */
#pragma once

#include "NonCopyable.h"
#include "FileLog.h"
#include <string>

namespace tmms
{
    namespace base
    {
        enum LogLevel       // 日志等级，常量枚举一般K开头
        {
            KTrace,
            KDebug,
            KInfo,
            KWarn,
            KError,
            KMaxNumOfLogLevel,
        };

        class Logger : public NonCopyable
        {
        public:
            Logger(FileLog::ptr &log);
            ~Logger() = default;
        
        public:
            void SetLogLevel(const LogLevel &level);     // 设置日志等级
            LogLevel GetLogLevel() const;
            void Write(const std::string &msg);          // 写日志
        
        private:
            LogLevel level_{KDebug};
            FileLog::ptr log_;
        };
        
        

    } // namespace base
    
} // namespace tmms
