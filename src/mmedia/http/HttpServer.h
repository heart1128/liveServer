/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-10 14:41:20
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-02 17:33:43
 * @FilePath: /tmms/src/mmedia/rtmp/HttpServer.h
 * @Description:  learn 
 */
#pragma once
#include "mmedia/http/HttpHandler.h"
#include "network/net/TcpConnection.h"
#include "network/TcpServer.h"

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;

        
        /// @brief 同样也是处理回调
        class HttpServer : public TcpServer
        {
        public:
            HttpServer(EventLoop *loop, const InetAddress& local, HttpHandler* handler = nullptr);
            ~HttpServer();
        
        public:
            void Start() override;
            void Stop() override;

        private:
            void OnNewConnection(const TcpConnectionPtr &conn); 
            void OnConnectionDestroy(const TcpConnectionPtr &conn);
            void OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf);
            void OnWriteComplete(const ConnectionPtr &conn);
            void OnActive(const ConnectionPtr &conn);

            HttpHandler *http_handler_{nullptr};
        };
        
        
        
    } // namespace mm
    
} // namespace tmms
