/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 11:24:44
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 13:40:57
 * @FilePath: /tmms/src/network/UdpClient.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "network/net/EventLoop.h"
#include "network/net/UdpSocket.h"
#include <functional>

namespace tmms
{
    namespace network
    {
        using ConnectedCallback = std::function<void(const UdpSocketPtr&, bool)>;

        class UdpClient : public UdpSocket
        {
        public:
            UdpClient(EventLoop *loop, const InetAddress &server);
            virtual ~UdpClient();
        
        public:
            void Connect();
            void SetConnectedCallback(const ConnectedCallback &cb);
            void SetConnectedCallback(ConnectedCallback &&cb);
            void ConnectInLoop();
            void Send(std::list<UdpBufferNodePtr> &list);
            void Send(const char* buf, size_t size);
            void OnClose() override;
        
        private:
            bool connected_{false};
            InetAddress server_addr_;   // 服务端地址
            ConnectedCallback connected_cb_;
            struct sockaddr_in6 sock_addr_; // 服务端的socket地址结构
            socklen_t sock_len_{sizeof(struct sockaddr_in6)};
        };


    } // namespace network

} // namespace tmms
