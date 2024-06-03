/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 14:08:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-02 14:44:04
 * @FilePath: /liveServer/src/base/FileMgr.cpp
 * @Description:  learn 
 */
#include "FileMgr.h"
#include "TTime.h"
#include "StringUtils.h"
#include <sstream>
#include <iostream>

using namespace tmms::base;

namespace       // 匿名命名空间，在这个文件有效
{
    // 创建一个全局的，每次返回空的就不用创建了
    static tmms::base::FileLog::ptr file_log_nullptr;
}

/// @brief 检测是否要切分，按小时或者日期检测
void FileMgr::OnCheck()
{
    bool day_change {false};
    bool hour_change {false};
    bool minute_change{false};
    int year = 0, month = 0, day = 0;
    int hour = 0, minute = 0, second = 0;
    TTime::Now(year, month, day, hour, minute, second);

    if(last_day_ == -1)     // 未初始化，进行初始化
    {
        last_day_ = day;
        last_hour_ = hour;
        last_year_ = year;
        last_month_ = month;
        last_minute_ = minute;
    }

    if(last_day_ != day)    // 判断时间是否改变
    {
        day_change = true;
    }
    if(last_hour_ != hour)
    {
        hour_change = true;
    }
    if(last_minute_ != minute)
    {
        minute_change = true;
    }

    if(!day_change && !hour_change && !minute_change) // 天和小时都没有改变，说明不需要改变文件切分
    {
        return;
    }

    // 其中满足一个就进行切分
    std::lock_guard<std::mutex> lk(lock_);
    for(auto &log : logs_)
    {   
        std::cout << log.second->GetRotateType() << std::endl;
        // 如果设置了按小时切分
        if(hour_change && log.second->GetRotateType() == kRotateHour)
        {
            RotateHours(log.second);
        }
        // 按天数切分
        if(day_change && log.second->GetRotateType() == kRotateDay)
        {
            RotateDays(log.second);
        }
        // 按分钟切分
        if(minute_change && log.second->GetRotateType() == kRotateMinute)
        {
            RotateMinute(log.second);
        }
    }

    last_day_ = day;
    last_hour_ = hour;
    last_year_ = year;
    last_month_ = month;
    last_minute_ = minute;
}

/// @brief 在map中查找日志文件，如果有就返回日志指针，如果没有就创建新的返回
/// @param file_name 
/// @return 
FileLog::ptr FileMgr::GetFileLog(const std::string &file_name)
{
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = logs_.find(file_name);
    if(iter != logs_.end())         // 存在这个日志
    {
        return iter->second;
    }

    // 没有找到这个日志，就创建新的
    FileLog::ptr log = std::make_shared<FileLog>();
    if(!log->Open(file_name))
    {
        return file_log_nullptr;    // 打开失败返回空的
    }

    logs_.emplace(file_name, log);
    return log;
}

/// @brief 直接中map中删除就行
/// @param log 
void FileMgr::RemoveFileLog(const FileLog::ptr &log)
{
    std::lock_guard<std::mutex> lk(lock_);
    
    logs_.erase(log->FilePath());
}

/// @brief 按天切分文件
/// @param file 
void FileMgr::RotateDays(const FileLog::ptr &file)
{
    if(file->FileSize() > 0)
    {
        char buf[128] = {0,};
        sprintf(buf, "_%04d-%02d-%02d", last_year_, last_month_, last_day_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::FileNameExt(file_path);

        std::ostringstream ss;
        ss << path
            << file_name
            << buf
            << file_ext;
        
        file->Rotate(ss.str());
    }
}

/// @brief 按小时切分文件
/// @param file 
void FileMgr::RotateHours(FileLog::ptr &file)
{
    if(file->FileSize() > 0)
    {
        char buf[128] = {0,};
        sprintf(buf, "_%04d-%02d-%02dT%02d", last_year_, last_month_, last_day_, last_hour_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::FileNameExt(file_path);

        std::ostringstream ss;
        ss << path
            << file_name
            << buf
            << file_ext;
        
        file->Rotate(ss.str());
    }
}

/// @brief 测试使用，按分钟切分
/// @param file 
void FileMgr::RotateMinute(FileLog::ptr &file)
{
    if(file->FileSize() > 0)
    {
        char buf[128] = {0,};
        sprintf(buf, "_%04d-%02d-%02dT%02d%02d", last_year_, last_month_, last_day_, last_hour_, last_minute_);

        std::string file_path = file->FilePath();
        std::string path = StringUtils::FilePath(file_path);
        std::string file_name = StringUtils::FileName(file_path);
        std::string file_ext = StringUtils::FileNameExt(file_path);

        std::ostringstream ss;
        ss << path
            << file_name
            << buf
            << file_ext;
        
        file->Rotate(ss.str());
    }
}
