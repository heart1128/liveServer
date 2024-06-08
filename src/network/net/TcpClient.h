/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-08 08:56:30
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-08 08:59:23
 * @FilePath: /tmms/src/network/net/TcpClient.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"
#include <functional>

namespace tmms
{
    namespace network
    {
        enum
        {
            kTcpConStatusInit = 0,
            kTcpConStatusConnecting = 1,
            kTcpConStatusConnected = 2,
            kTcpConStatusDisConnected = 3,
        };

        using ConnectionCallback = std::function<void (const TcpConnectionPtr &con, bool)>; // 连接的是哪个tcpconnection，是否连接

        class TcpClient : public TcpConnection
        {
        public:
            TcpClient(EventLoop *loop, const InetAddress &server); // loop要有，类是继承Event的，加入监听
            virtual ~TcpClient();

        public:
            void SetConnectCallback(const ConnectionCallback &cb);
            void SetConnectCallback(ConnectionCallback &&cb);
            void Connect();

            void OnRead() override;
            void OnWrite() override;
            void OnClose() override;

            void Send(std::list<BufferNodePtr>&list);
            void Send(const char *buf,size_t size);
        
        private:
            void ConnectInLoop();
            void UpdateConnectionStatus();        
            bool CheckError();

            InetAddress server_addr_;   // 服务端地址
            int32_t status_{kTcpConStatusInit};
            ConnectionCallback connected_cb_;   //连接回调
        };
        

    } // namespace network
    
} // namespace tmms
