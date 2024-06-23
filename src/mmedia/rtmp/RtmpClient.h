
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 11:13:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 15:42:39
 * @FilePath: /tmms/src/mmedia/rtmp/RtmpClient.h
 * @Description:  learn 
 */
#pragma once
#include "network/TcpClient.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "mmedia/rtmp/RtmpHandler.h"
#include <memory>
#include <functional>

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;
        class RtmpClient
        {
        public:
            RtmpClient(EventLoop *loop, RtmpHandler *handler);
            ~RtmpClient();

        public:
 
            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);

            void Play(const std::string &url); // 播放（拉流）
            void Publish(const std::string &url); // 推流
            void Send(PacketPtr &&data); // 推流发送


        private:
            void OnWriteComplete(const TcpConnectionPtr &conn); // 写完成回调
            void OnConnection(const TcpConnectionPtr &conn, bool connected); // 连接回调
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &uf);
            
            bool ParseUrl(const std::string &url);
            void CreateTcpClient();

            EventLoop *loop_{nullptr};
            InetAddress addr_;  // 服务端地址
            RtmpHandler *handler_{nullptr};
            TcpClientPtr tcp_client_;   // 使用tcpclient进行数据的连接接受发送
            std::string url_;
            bool is_player_{false};
            CloseConnectionCallback close_cb_; 
        };
    } // namespace mm
    
} // namespace tmms
