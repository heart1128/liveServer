/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 10:36:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 14:14:16
 * @FilePath: /tmms/src/base/Config.cpp
 * @Description:  learn 
 */
#include "Config.h"
#include "LogStream.h"
#include <fstream>

using namespace tmms::base;


bool Config::LoadConfig(const std::string & file)
{
    LOG_DEBUG << "config file : " << file;

    Json::Value root;
    Json::CharReaderBuilder reader;
    std::ifstream in(file);
    std::string err;
    // 用流构建解析
    auto ok = Json::parseFromStream(reader, in, &root, &err);
    if(!ok)       // 解析json文件root到root
    {
        LOG_DEBUG << "config file:" << file << " parse error!";
        return false;
    }

    Json::Value nameObj = root["name"];
    if(!nameObj.isNull())
    {
        name_ = nameObj.asString();
    }
    Json::Value cpusObj = root["cpu_start"];
    if(!cpusObj.isNull())
    {
        cpu_start_ = cpusObj.asInt();
    }
    Json::Value threadsObj = root["threads"];
    if(!threadsObj.isNull())
    {
        name_ = threadsObj.asInt();
    }
    Json::Value logObj = root["log"];
    if(!logObj.isNull())
    {
        if(!ParseLogInfo(logObj))
        {
            return false;
        }
    }

    return true;
}

bool Config::ParseLogInfo(const Json::Value &root)
{
    log_info_ = std::make_shared<LogInfo>();

    Json::Value levelObj = root["level"];
    if(!levelObj.isNull())
    {
        // 这个=重载默认用的就是&&移动构造
        std::string level = levelObj.asString();
        if(level == "TRACE")
        {
            log_info_->level = KTrace;
        }
        if(level == "DEBUG")
        {
            log_info_->level = KDebug;
        }
        if(level == "INFO")
        {
            log_info_->level = KInfo;
        }
        if(level == "WARN")
        {
            log_info_->level = KWarn;
        }
        if(level == "ERROR")
        {
            log_info_->level = KError;
        }
    }
    else
    {
        LOG_DEBUG << "parse log level error!";
        return false;
    }

    Json::Value rtObj = root["rotate"];
    if(!rtObj.isNull())
    {
        // 这个=重载默认用的就是&&移动构造
        std::string rt = rtObj.asString();
        if(rt == "DAY")
        {
            log_info_->rotate_type = kRotateDay;
        }
        if(rt == "HOUR")
        {
            log_info_->rotate_type = kRotateHour;
        }
        if(rt == "MINUTE")
        {
            log_info_->rotate_type = kRotateMinute;
        }
    }
    else
    {
        LOG_DEBUG << "parse log level rotate!";
        return false;
    }


    Json::Value nameObj = root["name"];
    if(!nameObj.isNull())
    {
        // 这个=重载默认用的就是&&移动构造
        log_info_->name = nameObj.asString();
    }
    else
    {
        LOG_DEBUG << "parse log name error!";
        return false;
    }

    Json::Value pathObj = root["path"];
    if(!pathObj.isNull())
    {
        // 这个=重载默认用的就是&&移动构造
        log_info_->path = pathObj.asString();
    }
    else
    {
        LOG_DEBUG << "parse log path error!";
        return false;
    }

    return true;
}

LogInfo::ptr& Config::GetLogInfo()
{
    return log_info_;
}



///////////////////////  ConfigMgr

/// @brief 
/// @param file  
/// @return 
bool ConfigMgr::LoadConfig(const std::string& file)
{
    LOG_DEBUG << "load config file:" << file; 
    Config::ptr config = std::make_shared<Config>();
    // 临时变量加载配置，加载成功进行替换热更新
    if(config->LoadConfig(file))
    {
        std::lock_guard<std::mutex> lk(lock_);
        config_ = config;
        return true;
    }
        // 加载失败log打印在LoadConfig()打印了，这里不用再打印
    return false;
}

Config::ptr ConfigMgr::GetConfig()
{
    // 多线程共用一个mgr，上锁
    std::lock_guard<std::mutex> lk(lock_);
    return config_;
}
