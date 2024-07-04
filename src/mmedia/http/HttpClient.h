
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-23 11:13:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-03 18:23:45
 * @FilePath: /tmms/src/mmedia/http/HttpClient.h
 * @Description:  learn 
 */
#pragma once
#include "network/TcpClient.h"
#include "network/net/EventLoop.h"
#include "network/base/InetAddress.h"
#include "mmedia/http/HttpHandler.h"
#include "mmedia/http/HttpRequest.h"
#include <memory>
#include <functional>

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;
        class HttpClient
        {
        public:
            HttpClient(EventLoop *loop, HttpHandler *handler);
            ~HttpClient();

        public:
 
            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);

            void Get(const std::string &url); 
            void Post(const std::string &url, const PacketPtr& packet); 


        private:
            void OnWriteComplete(const TcpConnectionPtr &conn); // 写完成回调
            void OnConnection(const TcpConnectionPtr &conn, bool connected); // 连接回调
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &uf);
            
            bool ParseUrl(const std::string &url);
            void CreateTcpClient();

            EventLoop *loop_{nullptr};
            InetAddress addr_;  // 服务端地址
            HttpHandler *handler_{nullptr};
            TcpClientPtr tcp_client_;   // 使用tcpclient进行数据的连接接受发送
            std::string url_;
            bool is_post_{false};
            CloseConnectionCallback close_cb_; 
            HttpRequestPtr request_;
            PacketPtr out_packet_;

        };
    } // namespace mm
    
} // namespace tmms
