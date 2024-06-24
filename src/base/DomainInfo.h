/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 17:56:39
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 17:59:55
 * @FilePath: /tmms/src/base/DomainInfo.h
 * @Description:  learn 
 */
#pragma once
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>

namespace tmms
{
    namespace base
    {
        class AppInfo;
        using AppInfoPtr = std::shared_ptr<AppInfo>;

        class DomainInfo
        {
        public:
            DomainInfo(/* args */) = default;
            ~DomainInfo() = default;

        public:
            const std::string& DomainName() const; // 返回域名
            const std::string& Type() const;
            bool ParseDomainInfo(const std::string &file);
            AppInfoPtr GetAppInfo(const std::string& app_name);
        
        private:
            std::string name_; // domain name
            std::string type_;
            std::mutex lock_;
            std::unordered_map<std::string, AppInfoPtr> appinfos_;
        };
        
    } // namespace base
    
} // namespace tmms
    