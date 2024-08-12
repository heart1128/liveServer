/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 11:52:01
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 18:30:20
 * @FilePath: /liveServer/src/live/WebrtcService.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <string>
#include "mmedia/webrtc/WebrtcHandler.h"
#include "mmedia/http/HttpHandler.h"
#include "live/user/WebrtcPlayUser.h"
#include "base/Singleton.h"

namespace tmms
{
    namespace live
    {
        using namespace mm;
        using WebrtcPlayUserPtr = std::shared_ptr<WebrtcPlayUser>;
        class WebrtcService : public WebrtcHandler, public HttpHandler
        {
        public:
            WebrtcService() = default;
            ~WebrtcService() = default;

            // webrtc上层服务，响应下层接口
            void OnStun(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) override;
            void OnDtls(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) override;
            void OnRtp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) override;
            void OnRtcp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf) override;

            void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet) override;

            // 以下接口都用不到，但是要实现，所以放空
            void OnSent(const TcpConnectionPtr &conn) override {}
            bool OnSentNextChunk(const TcpConnectionPtr &conn) override{ return false;}
            void OnNewConnection(const TcpConnectionPtr &conn) override {}
            void OnConnectionDestroy(const TcpConnectionPtr &conn)override{}
            void OnRecv(const TcpConnectionPtr& conn, const PacketPtr &data)override{} 
            void OnRecv(const TcpConnectionPtr& conn, PacketPtr &&data)override{}
            void OnActive(const ConnectionPtr &conn)override{}

            /**
             * @description: 主动通知用户拉数据
             */            
            void Push2Players();
        
        private:
            std::string GetSessionNameFromUrl(const std::string &url);
            std::mutex lock_;  // 多线程操作map
            std::mutex user_lock_; // 不同用户之间的锁
            std::unordered_map<std::string, WebrtcPlayUserPtr> name_users_; // 用户名对player
            std::unordered_map<std::string, WebrtcPlayUserPtr> users_;      // 在udp没有username的情况下使用
        };

        #define sWebrtcService tmms::base::Singleton<tmms::live::WebrtcService>::Instance()
        
    } // namespace live
    
} // namespace tmms
