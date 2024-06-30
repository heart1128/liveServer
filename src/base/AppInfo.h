/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 18:10:32
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-28 17:47:53
 * @FilePath: /liveServer/src/base/AppInfo.h
 * @Description:  learn 
 */
#pragma once
#include <string>
#include <json/json.h>
#include <memory>

namespace tmms
{
    namespace base
    {
        class DomainInfo;
        class AppInfo;
        
        using AppInfoPtr = std::shared_ptr<AppInfo>;
        class AppInfo
        {
        public:
            explicit AppInfo(DomainInfo &d);
            ~AppInfo() = default;

            bool ParseAppInfo(Json::Value &root);
        
        public:
            DomainInfo &domain_info; // 引用数据成员一定要在构造函数的序列化参数初始化
            std::string domain_name;
            std::string app_name;
            uint32_t max_buffer{1000}; //默认1000帧
            bool rtmp_support{false}; // 默认不支持rtmp
            bool flv_support{false};
            bool hls_support{false};
            uint32_t content_latency{3 * 1000}; // 直播延时，单位ms
            uint32_t stream_idle_time{30 * 1000}; // 流间隔最大时长
            uint32_t stream_timeout_time{30 * 1000}; // 流超时时间
        };
        
        
    } // namespace base
    
} // namespace tmms
