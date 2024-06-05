/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 14:15:41
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-05 15:10:16
 * @FilePath: /tmms/src/network/base/SocketOpt.h
 * @Description:  learn 
 */
#pragma once

#include "InetAddress.h"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <fcntl.h>
#include <memory>

namespace tmms
{
    namespace base
    {
        class SocketOpt
        {
        public:
            SocketOpt(int sock, bool v6 = false);
            ~SocketOpt() = default;
        
        public:
            ////// 服务端
            static int CreateNonblockingTcpSocket(int family);
            static int CreateNonblockingUdpSocket(int family);
            int BindAddress(const InetAdress &localaddr);
            int Listen();
            int Accept(InetAdress *peeraddr);
        
        public:
            ////// 客户端
            int Connect(const InetAdress &addr);
        
        public:
            InetAdress::ptr GetLocalAddr();   // 本地地址
            InetAdress::ptr GetPeerAddr();    // 远端地址
            void SetTcpNoDelay(bool on);
            void SetReuseAddr(bool on);
            void SetReusePort(bool on);
            void SetNonBlocking(bool on);

        private:
            int sock_{-1};
            bool is_v6_{false};
        };

        
    } // namespace base
    
} // namespace tmms
