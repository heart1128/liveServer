/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-25 20:25:32
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-17 16:51:04
 * @FilePath: /liveServer/src/base/AppInfo.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
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
        class Target;

        using TargetPtr = std::shared_ptr<Target>;
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

            std::vector<TargetPtr> pulls;
        };
        
        
    } // namespace base
    
} // namespace tmms
