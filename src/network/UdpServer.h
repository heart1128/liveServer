/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 14:46:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 19:09:35
 * @FilePath: /liveServer/src/network/UdpServer.h
 * @Description:  learn 
 */
#pragma once
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"
#include <functional>

namespace tmms
{
    namespace network
    {
        using namespace tmms::base;
        
        class UdpServer : public UdpSocket
        {
        public:
            UdpServer(EventLoop *loop, const InetAddress &server);
            virtual ~UdpServer();
        
        public:
            void Start();
            void Stop();
        private:
            void Open();

            InetAddress server_;
        };
        
    } // namespace netwrok
    
} // namespace tmms
