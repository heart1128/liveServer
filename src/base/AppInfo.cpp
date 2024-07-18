/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-25 20:25:32
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-17 16:55:42
 * @FilePath: /liveServer/src/base/AppInfo.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 18:16:49
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-30 10:27:51
 * @FilePath: /liveServer/src/base/AppInfo.cpp
 * @Description:  learn 
 */
#include "AppInfo.h"
#include "base/DomainInfo.h"
#include "base/LogStream.h"
#include "Target.h"

using namespace tmms::base;

AppInfo::AppInfo(DomainInfo &d)
:domain_info(d)
{
}

/// @brief 被DomainInfo调用，进一步解析app数组
/// @param root 
/// @return 
bool AppInfo::ParseAppInfo(Json::Value &root)
{
    Json::Value nameObj = root["name"];
    if(!nameObj.isNull())
    {
        app_name = nameObj.asString();
    }

    Json::Value maxBufferObj = root["max_buffer"];
    if(!maxBufferObj.isNull())
    {
        max_buffer = maxBufferObj.asUInt();
    }

    Json::Value hlsObj = root["hls_support"];
    if(!hlsObj.isNull())
    {
        hls_support = hlsObj.asString() == "on" ? true : false;
    }

    Json::Value flvObj = root["flv_support"];
    if(!flvObj.isNull())
    {
        flv_support = flvObj.asString() == "on" ? true : false;
    }

    Json::Value rtmpObj = root["rtmp_support"];
    if(!rtmpObj.isNull())
    {
        rtmp_support = rtmpObj.asString() == "on" ? true : false;
    }

    Json::Value clObj = root["content_latency"];
    if(!clObj.isNull())
    {
        content_latency = clObj.asUInt() * 1000; // ms
    }

    Json::Value sitObj = root["stream_idle_time"];
    if(!sitObj.isNull())
    {
        stream_idle_time = sitObj.asUInt();
    }

    Json::Value sttObj = root["stream_timeout_time"];
    if(!sttObj.isNull())
    {
        stream_timeout_time = sttObj.asUInt();
    }

    Json::Value pullObj = root["pull"];  // 回源
    if(!pullObj.isNull() &&pullObj.isArray())
    {
        for(auto &a : pullObj)
        {
            TargetPtr p = std::make_shared<Target>(domain_name, app_name);
            p->ParseTarget(a);
        }
    }

    LOG_INFO << "app name:" << app_name
            << " max_buffer:" << max_buffer
            << " content_latency:" << content_latency
            << " stream_idle_time:" << stream_idle_time
            << " stream_timeout_time:" << stream_timeout_time
            << " rtmp_support:" << rtmp_support
            << " flv_support:" << flv_support
            << " hls_support:" << hls_support;
    
    return true;
}

