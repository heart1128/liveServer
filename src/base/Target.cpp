#include "Target.h"
#include "base/LogStream.h"
#include "StringUtils.h"

using namespace tmms::base;

Target::Target(const std::string &session_name)
{
    // heart.com/live/test
    auto list = StringUtils::SplitString(session_name, "/");
    if(list.size() == 3)
    {
        domain_name = list[0];
        app_name = list[1];
        stream_name = list[2];
    }
}

Target::Target(const std::string &dn, const std::string &an)
:domain_name(dn), app_name(an)
{
}

bool Target::ParseTarget(Json::Value &root)
{
    Json::Value &proObj = root["protocol"];
    if(!proObj.isNull())
    {
        protocol = std::move(proObj.asString());
    }

    Json::Value &urlObj = root["url"];
    if(!urlObj.isNull())
    {
        url = std::move(urlObj.asString());
    }

    Json::Value &paramObj = root["param"];
    if(!paramObj.isNull())
    {
        param = std::move(paramObj.asString());
    }

    Json::Value &hostObj = root["host"];
    if(!hostObj.isNull())
    {
        remote_host = std::move(hostObj.asString());
        // 可能是ip:port，查找一下
        auto pos = remote_host.find_first_of(":");
        if(pos != std::string::npos)
        {
            remote_host = remote_host.substr(0, pos);
            remote_port = std::atoi(remote_host.substr(pos + 1).c_str());
        }
        else    // 设置默认端口
        {
            if(protocol == "rtmp")
            {
                remote_port = 1935;
            }
            else if(protocol == "http")
            {
                remote_port = 80;
            }
            else if(protocol == "pav")  // 私有协议，用一个大端口，不占用常用端口
            {
                remote_port = 22000;
            }
        }
    }
    else  // 没有host应该有url
    {
        ParseTargetUrl(url);
    }

    Json::Value &portObj = root["port"];
    if(!portObj.isNull())
    {
        remote_port = portObj.asUInt();
    }

    Json::Value &intervaltObj = root["interval"];
    if(!intervaltObj.isNull())
    {
        interval = intervaltObj.asUInt();
    }

    Json::Value &max_retryObj = root["max_retry"];
    if(!max_retryObj.isNull())
    {
        max_retry = max_retryObj.asUInt();
    }

    LOG_TRACE << "target:" << session_name
            << " protocol:" << protocol
            << " url:" << url
            << " param:" << param
            << " host:" << remote_host
            << " port:" << remote_port
            << " max retry:" << max_retry
            << " interval:" << interval;

    return true;
}

void Target::ParseTargetUrl(const std::string &url)
{
    if(protocol == "http")
    {
        // http://ip or:port/
        auto pos = url.find_first_of("/", 7);
        if(pos != std::string::npos)
        {
            auto host = url.substr(7, pos);
            auto pos1 = remote_host.find_first_of(":");
            if(pos1 != std::string::npos)
            {
                remote_host = remote_host.substr(0, pos);
                remote_port = std::atoi(remote_host.substr(pos + 1).c_str());
            }
            else
            {
                remote_host = host;
                remote_port = 80;
            }
        }
        else // 后面只有host
        {
            remote_host = url.substr(7);
            remote_port = 80;
        }
    }
}
