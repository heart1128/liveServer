/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 18:25:10
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 20:38:21
 * @FilePath: /liveServer/src/base/LogStream.h
 * @Description:  learn 
 */
#pragma once
#include "Logger.h"
#include <sstream>

namespace tmms
{
   namespace base
   {
        extern Logger *g_logger;
        class LogStream
        {
        public:
            LogStream(Logger* logger, const char* file, int line, LogLevel logLevel, const char* func = nullptr);
            ~LogStream();
        
            template<class T>
            LogStream& operator << (const T& value);
        
        private:
            std::ostringstream stream_;
            Logger *logger_{nullptr};
        };

        /// @brief 重载<<直接到ostringstream流中
        /// @tparam T 
        /// @param value 
        /// @return 
        template <class T>
        LogStream &LogStream::operator<<(const T &value)
        {
            stream_ << value;
            return *this;
        }

   } // namespace base

} // namespace tmms


#define LOG_TRACE   \
    if(tmms::base::g_logger->GetLogLevel() <= KTrace)   \
        tmms::base::LogStream(g_logger, __FILE__, __LINE__, tmms::base::KTrace, __func__) 

#define LOG_DEBUG   \
    if(tmms::base::g_logger->GetLogLevel() <= KDebug)   \
        tmms::base::LogStream(g_logger, __FILE__, __LINE__, tmms::base::KDebug, __func__) 

#define LOG_WARN   \
        tmms::base::LogStream(g_logger, __FILE__, __LINE__, tmms::base::KWarn) 

#define LOG_INFO   \
    if(tmms::base::g_logger->GetLogLevel() <= KInfo)   \
        tmms::base::LogStream(g_logger, __FILE__, __LINE__, tmms::base::KTrace) 

#define LOG_ERROR  \
    if(tmms::base::g_logger->GetLogLevel() <= KError)   \
        tmms::base::LogStream(g_logger, __FILE__, __LINE__, tmms::base::KError, __func__) 

// define不能在文件的末尾，至少加一个空行