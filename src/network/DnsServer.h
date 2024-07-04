/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-08 10:18:00
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 08:55:31
 * @FilePath: /tmms/src/network/DnsServer.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "base/NonCopyable.h"
#include "base/Singleton.h"
#include "network/base/InetAddress.h"
#include <unordered_map>
#include <mutex>
#include <thread>
#include <vector>
#include <string>

namespace tmms
{
    namespace network
    {
        using namespace base;

        class DnsServer : public base::NonCopyable      // 单例
        {
        public:
            DnsServer() = default;
            ~DnsServer() = default;
        
        public:
            void AddHost(const std::string& host);
            InetAddress::ptr GetHostAddress(const std::string &host,int index);
            std::vector<InetAddress::ptr> GetHostAddress(const std::string &host);
            void UpdateHost(const std::string &host,std::vector<InetAddress::ptr> &list);
            std::unordered_map<std::string,std::vector<InetAddress::ptr>> GetHosts();
            void SetDnsServiceParam(int32_t interval,int32_t sleep,int32_t retry);
            void Start();
            void Stop();
            void OnWork();
            static void GetHostInfo(const std::string&host,std::vector<InetAddress::ptr>&list);
        
        private:
            std::thread thread_;
            bool running_{false};
            std::mutex lock_;
            // 域名，地址对
            std::unordered_map<std::string , std::vector<InetAddress::ptr>> hosts_info_;
            int32_t retry_{3};
            int32_t sleep_{200}; // ms
            int32_t interval_{180 * 1000}; // ms，6mintue
        };
        
        #define sDnsService tmms::base::Singleton<tmms::network::DnsServer>::Instance()
    } // namespace network
    
} // namespace tmms
