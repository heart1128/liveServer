/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 14:00:23
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-02 15:01:15
 * @FilePath: /liveServer/src/base/FileMgr.h
 * @Description:  learn 
 */
#pragma once


#include "FileLog.h"
#include "Singleton.h"  // 管理类都是用的单例
#include <memory>
#include <unordered_map>
#include <mutex>

namespace tmms
{
    namespace base
    {
        class FileMgr : public NonCopyable
        {
        public:
            FileMgr() = default;
            ~FileMgr() = default;
        
        public:
            void OnCheck();
            FileLog::ptr GetFileLog(const std::string &file_name);
            void RemoveFileLog(const FileLog::ptr &log);
            void RotateDays(const FileLog::ptr &file);
            void RotateHours(FileLog::ptr &file);
            void RotateMinute(FileLog::ptr &file);  // 测试用
        
        private:
            std::unordered_map<std::string, FileLog::ptr> logs_;        // <文件路径名，日志指针>
            std::mutex lock_;
            int last_day_{-1};      // 上一次保存日志的日
            int last_hour_{-1};
            int last_year_{-1};
            int last_month_{-1};
            int last_minute_{-1};
        }; 
    } // namespace base
    
} // namespace tmms

// 单例
#define sFileMgr tmms::base::Singleton<tmms::base::FileMgr>::Instance()