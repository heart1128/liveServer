/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 18:01:02
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 18:32:15
 * @FilePath: /tmms/src/base/DomainInfo.cpp
 * @Description:  learn 
 */
#include "DomainInfo.h"
#include "json/json.h"
#include "base/LogStream.h"
#include "base/AppInfo.h"
#include <fstream>

using namespace tmms::base;

const std::string &DomainInfo::DomainName() const
{
    return name_;
}

const std::string &DomainInfo::Type() const
{
    return type_;
}

bool DomainInfo::ParseDomainInfo(const std::string &file)
{
    LOG_DEBUG << "domain file : " << file;

    Json::Value root;
    Json::CharReaderBuilder reader;
    std::ifstream in(file);
    std::string err;
    // 用流构建解析
    auto ok = Json::parseFromStream(reader, in, &root, &err);
    if(!ok)       // 解析json文件root到root
    {
        LOG_DEBUG << "domain config file:" << file << " parse error.err: " << err;
        return false;
    }

    Json::Value domainObj = root["domain"];
    if(domainObj.isNull())
    {
        LOG_ERROR << "domain config invalid content.no domain.";
        return false;
    }
    // name
    Json::Value nameObj = domainObj["name"];
    if(!nameObj.isNull())
    {
        name_ = nameObj.asString();
    }
    // type
    Json::Value typeObj = domainObj["type"];
    if(!typeObj.isNull())
    {
        type_ = typeObj.asString();
    }
    // app
    Json::Value appsObj = domainObj["app"];
    if(appsObj.isNull())
    {
        LOG_ERROR << "domain config invalid content.no apps.";
        return false;
    }
    // app是数组
    for(auto &appObj : appsObj)
    {
        AppInfoPtr appinfo = std::make_shared<AppInfo>(*this);
        auto ret = appinfo->ParseAppInfo(appObj);
        if(ret)
        {
            std::lock_guard<std::mutex> lk(lock_);
            appinfos_.emplace(appinfo->app_name, appinfo);
        }
    }

    return true;
}

AppInfoPtr DomainInfo::GetAppInfo(const std::string &app_name)
{
    std::lock_guard<std::mutex> lk(lock_);

    auto iter = appinfos_.find(app_name);
    if(iter != appinfos_.end())
    {
        return iter->second;
    }
    return AppInfoPtr();
}
