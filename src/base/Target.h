#pragma once

#include <string>
#include <cstdint>
#include <memory>
#include <json/json.h>

/**
 * 
 * 回源配置，只是实现了业务配置的静态配置文件（正常是一个静态配置服务器） 
 *  - 还有通过API获取没实现
 *  - 通过机房局域网广播（如果局域网有一个机器拉到了流，局域网流量不计费，省钱）
*/

namespace tmms
{
    namespace base
    {
        class Target
        {
        public:
            Target(const std::string &session_name);
            Target(const std::string &domain_name, const std::string& app_name);
            ~Target() = default;

            bool ParseTarget(Json::Value &root);
            void ParseTargetUrl(const std::string &url);
            void SetStreamName();

            std::string remote_host;        // 回源远端主机的host
            unsigned short remote_port;
            std::string session_name;
            std::string domain_name;
            std::string app_name;
            std::string stream_name;
            std::string protocol;
            std::string url;   // rtmp不用,http用
            int32_t interval{1000}; // 重试间隔
            int32_t max_retry = 0;
            int32_t retry = 0;
            std::string param;
        };
        
        
    } // namespace mm
    
} // namespace tmms
