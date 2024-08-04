/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 15:11:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 15:24:45
 * @FilePath: /liveServer/src/mmedia/webrtc/WebrtcServer.h
 * @Description:  learn 
 */
#pragma once

#include "network/UdpServer.h"
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "WebrtcHandler.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        using namespace network;
        class WebrtcServer
        {
        public:
            WebrtcServer(EventLoop *loop, const InetAddress &server, WebrtcHandler *handler);
            ~WebrtcServer() = default;
            
            void Start();

        private:
            void MessageCallback(const UdpSocketPtr &socket, const InetAddress &addr, MsgBuffer &buf);
            bool IsDtls(MsgBuffer &buf);
            bool IsStun(MsgBuffer &buf);
            bool IsRtp(MsgBuffer &buf);
            bool IsRtcp(MsgBuffer &buf);

            WebrtcHandler *handler_{nullptr};
            std::shared_ptr<UdpServer> udp_server_;
        };
        
        
    } // namespace mm
    
} // namespace tmms
