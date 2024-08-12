/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 15:11:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 19:11:52
 * @FilePath: /liveServer/src/mmedia/webrtc/WebrtcServer.h
 * @Description:  learn 
 */
#pragma once

#include "network/UdpServer.h"
#include "network/net/UdpSocket.h"
#include "network/net/EventLoop.h"
#include "mmedia/base/AVTypes.h"
#include "mmedia/base/Packet.h"
#include "WebrtcHandler.h"
#include <cstdint>
#include <list>

namespace tmms
{
    namespace mm
    {
        using namespace network;
        using namespace base;
        class WebrtcServer
        {
        public:
            WebrtcServer(EventLoop *loop, const InetAddress &server, WebrtcHandler *handler);
            ~WebrtcServer() = default;
            
            void Start();
            // 发送rtp数据的
            void SendPacket(const PacketPtr &packet);
            void SendPacket(std::list<PacketPtr> &list);

        private:
            void MessageCallback(const UdpSocketPtr &socket, const InetAddress &addr, MsgBuffer &buf);
            bool IsDtls(MsgBuffer &buf);
            bool IsStun(MsgBuffer &buf);
            bool IsRtp(MsgBuffer &buf);
            bool IsRtcp(MsgBuffer &buf);
            void OnSend();
            void WriteComplete(const UdpSocketPtr &socket);

            WebrtcHandler *handler_{nullptr};
            std::shared_ptr<UdpServer> udp_server_;

            std::list<PacketPtr> sending_; // 发送消息的队列，因为没有连接
            std::list<PacketPtr> out_waiting_;
            std::list<UdpBufferNodePtr> udp_outs_;
            bool b_sending_{false};
            std::mutex lock_;
        };
        
        
    } // namespace mm
    
} // namespace tmms
