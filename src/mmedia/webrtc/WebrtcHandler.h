/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 11:53:24
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 11:54:51
 * @FilePath: /liveServer/src/mmedia/webrtc/WebrtcHandler.h
 * @Description:  learn 
 */
#pragma once

#include "base/NonCopyable.h"
#include "network/UdpServer.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        class WebrtcHandler : public base::NonCopyable
        {
        public:
            WebrtcHandler(/* args */) = default;
            ~WebrtcHandler() = default;

            virtual void OnStun(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) = 0;
            virtual void OnDtls(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) = 0;
            virtual void OnRtp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) = 0;
            virtual void OnRtcp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) = 0;
        };
        
        
    } // namespace mm
    
} // namespace tmms
