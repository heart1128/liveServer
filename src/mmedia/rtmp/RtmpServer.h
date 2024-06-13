/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-10 14:41:20
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-10 14:52:37
 * @FilePath: /tmms/src/mmedia/rtmp/RtmpServer.h
 * @Description:  learn 
 */
#pragma once
#include "mmedia/rtmp/RtmpHandler.h"
#include "network/net/TcpConnection.h"
#include "network/TcpServer.h"

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;

        class RtmpServer : public TcpServer
        {
        public:
            RtmpServer(EventLoop *loop, const InetAddress& local, RtmpHandler* handler = nullptr);
            ~RtmpServer();
        
        public:
            void Start() override;
            void Stop() override;

        private:
            void OnNewConnection(const TcpConnectionPtr &conn); 
            void OnConnectionDestroy(const TcpConnectionPtr &conn);
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf);
            void OnWriteComplete(const ConnectionPtr &conn);
            void OnActive(const ConnectionPtr &conn);

            RtmpHandler *rtmp_handler_{nullptr};
        };
        
        
        
    } // namespace mm
    
} // namespace tmms
