/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-04 20:42:25
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-05 15:02:33
 * @FilePath: /tmms/src/network/base/InetAddress.h
 * @Description:  learn 
 */
#pragma once

#include <sys/socket.h> // socket
#include <arpa/inet.h>  // 地址转换
#include <netinet/in.h>
#include <bits/socket.h> // socket数据类型
#include <string>
#include <memory>

namespace tmms
{
    namespace base
    {
        class InetAdress
        {
        public:
            using ptr = std::shared_ptr<InetAdress>;

            InetAdress(const std::string &ip, uint16_t port, bool bv6=false);
            InetAdress(const std::string &host, bool bv6=false);
            InetAdress() = default;
            ~InetAdress() = default;
        
        public:
            void SetHost(const std::string &host);
            void SetAddr(const std::string &addr);
            void SetPort(uint16_t port);
            void SetIsIPv6(bool is_v6);

            const std::string &IP() const;
            uint32_t IPv4() const;
            std::string ToIpPort() const;
            uint16_t Port() const;
            void GetSockAddr(struct sockaddr* saddr) const;
            static void GetIpAndPort(const std::string &host, std::string &ip, std::string &port);

            //test
            bool IsIpV6() const;
            bool IsWanIp() const;       // 广域网
            bool IsLanIp() const;       // 聚局域
            bool IsLoopbackIp() const;  // 回播测试

        private:
            uint32_t IPv4(const char* ip) const;
            std::string addr_;
            std::string port_;
            bool is_ipv6_{false}; // 默认不是ipv6
        };
        
    } // namespace network
    
} // namespace tmms

