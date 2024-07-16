/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 10:36:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 22:04:47
 * @FilePath: /liveServer/src/base/Config.cpp
 * @Description:  learn 
 */
#include "Config.h"
#include "LogStream.h"
#include "DomainInfo.h"
#include "AppInfo.h"
#include <fstream>
#include <sys/stat.h> // 文件属性
#include <unistd.h> 
#include <dirent.h> // 目录头文件

using namespace tmms::base;

namespace
{
    static ServiceInfoPtr service_info_nullptr;
}

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
    Json::Value cpus1Obj = root["cpus"];
    if(!cpus1Obj.isNull())
    {
        cpus_ = cpus1Obj.asInt();
    }
    Json::Value threadsObj = root["threads"];
    if(!threadsObj.isNull())
    {
        thread_num_ = threadsObj.asInt();
    }
    Json::Value logObj = root["log"];
    if(!logObj.isNull())
    {
        if(!ParseLogInfo(logObj))
        {
            return false;
        }
    }
    // 解析直播服务信息
    if(!ParseServiceInfo(root["services"]))
    {
        return false;
    }
    ParseDirectory(root["directory"]); // 

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

/// @brief 解析直播服务配置文件的目录，列表中可以是文件或者目录
/// @param root 
/// @return 
bool Config::ParseDirectory(const Json::Value &root)
{
    if(root.isNull() || !root.isArray())
    {
        return false;
    }
    for(const auto& d : root)
    {
        std::string path = d.asString();
        struct stat st;
        LOG_TRACE << "ParseDirectory path:" << path;
        auto ret = stat(path.c_str(), &st);
        LOG_TRACE << "ret:" << ret << " errno:" << errno;
        if(ret != -1)
        {
            // S_IFMT屏蔽与文件类型无关的标志位，剩下的判断文件类型
            if((st.st_mode & S_IFMT) == S_IFDIR)
            {
                ParseDomainPath(path);
            }
            else if((st.st_mode & S_IFMT) == S_IFREG) // 普通文件
            {
                ParseDomainFile(path);
            }
        }
    }
    return true;
}

/// @brief 递归处理目录下的配置文件
/// @param path 
/// @return 
bool Config::ParseDomainPath(const std::string &path)
{
    DIR *dp = nullptr;
    struct dirent *pp = nullptr;

    LOG_DEBUG << "parse domain path:" << path;
    dp = opendir(path.c_str()); // 打开目录
    if(!dp)// 打开失败
    {
        return false;
    }

    // 循环遍历目录下的文件
    while (true)
    {
        pp = readdir(dp);   // 从目录中读一个文件
        if(pp == nullptr)  // 为空表示读完了
        {
            break;
        }
        if(pp->d_name[0] == '.')    // 处理隐藏文件不处理
        {
            continue;
        }
        if(pp->d_type == DT_REG) // 是文件类型
        {
            if(path.at(path.size() - 1) != '/') // 要加前面的路径
            {
                ParseDomainFile(path + "/" + pp->d_name);
            }
            else
            {
                ParseDomainFile(path + pp->d_name);
            }
        }
    }
    
    closedir(dp);
    return true;
}

/// @brief 解析配置文件,domaininfo包含appinfo
/// @param file 
/// @return 
bool Config::ParseDomainFile(const std::string &file)
{
    LOG_DEBUG << "parse domain file:" << file;
    DomainInfoPtr d = std::make_shared<DomainInfo>();
    auto ret = d->ParseDomainInfo(file);
    if(ret)
    {
        std::lock_guard<std::mutex> lk(lock_);
        // 查找一下，找到了要删除，因为新的要覆盖
        auto iter = domaininfos_.find(d->DomainName());
        if(iter != domaininfos_.end())
        {
            domaininfos_.erase(iter);
        }
        domaininfos_.emplace(d->DomainName(), d); // 添加新的
    }
    return true;
}

LogInfo::ptr& Config::GetLogInfo()
{
    return log_info_;
}

///////////////////////  直播业务管理

const std::vector<ServiceInfoPtr> &Config::GetServiceInfos()
{
    return services_;
}

const ServiceInfoPtr &Config::GetServiceInfo(const std::string &protocol, const std::string &transport)
{
    for(auto &s : services_)
    {
        if(s->protocol == protocol && transport == s->transport)
        {
            return s;
        }
    }
    return service_info_nullptr;
}

bool Config::ParseServiceInfo(const Json::Value &serviceObj)
{
    if(serviceObj.isNull())
    {
        LOG_ERROR << " config no service section!";
        return false;
    }
    if(!serviceObj.isArray())
    {
        LOG_ERROR << " service section type is not array!";
        return false;
    }

    for(auto const& s : serviceObj)
    {
        ServiceInfoPtr sinfo = std::make_shared<ServiceInfo>();

        sinfo->addr = s.get("addr", "0.0.0.0").asString();
        sinfo->port = s.get("port", "0").asUInt();
        sinfo->protocol = s.get("portocol", "rtmp").asString();
        sinfo->transport = s.get("transport", "tcp").asString();

        LOG_INFO << "\nservice info addr:" << sinfo->addr
                << "\nport:" << sinfo->port
                << "\nprotocol:" << sinfo->protocol
                << "\ntransport:" << sinfo->transport;
        
        services_.emplace_back(sinfo);
    }
    return true;
}

AppInfoPtr Config::GetAppInfo(const std::string &domain, const std::string &app)
{
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = domaininfos_.find(domain);
    if(iter != domaininfos_.end()) 
    {
        return iter->second->GetAppInfo(app);
    }
    return AppInfoPtr();
}

DomainInfoPtr Config::GetDomainInfo(const std::string &domain)
{
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = domaininfos_.find(domain);
    if(iter != domaininfos_.end())
    {
        return iter->second;
    }
    return DomainInfoPtr();
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
