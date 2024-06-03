/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 17:00:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 11:34:27
 * @FilePath: /tmms/src/base/FileLog.h
 * @Description:  learn 
 */
#pragma once
#include <string>
#include <memory>

namespace tmms
{
    namespace base
    {   
        enum RotateType
        {
            kRotateNone,    // 不切分
            kRotateMinute,  // 按分钟切分，测试用，正常不会时间这么短
            kRotateHour,    // 按小时切分
            kRotateDay,     // 按天切分
            kRotateSize,    // 按文件大小切分 ----> 未实现
        };

        class FileLog
        {
        public:
            using ptr = std::shared_ptr<FileLog>;

        public:
            FileLog() = default;
            ~FileLog() = default;
        
        public:
            bool Open(const std::string &filePath);     // 打开日志文件
            size_t WriteLog(const std::string &msg);    // 写日志
            void Rotate(const std::string &file);       // 日志不能太大，日志切分，新的日志文件代替公共句柄
            void SetRotate(RotateType type);            // 切分日志的类型，时间或大小

            RotateType GetRotateType() const;
            int64_t FileSize() const;
            std::string FilePath() const;

        private:
            int fd_{-1};                // 文件描述符
            std::string file_path_; // 文件路径
            RotateType rotate_type_{kRotateNone};
        };
        
    } // namespace base
    
} // namespace tmms
