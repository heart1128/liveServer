#pragma once
#include "network/net/TcpConnection.h"
#include "network/net/EventLoop.h"
#include "network/net/Acceptor.h"
#include "network/base/InetAddress.h"
#include <functional>
#include <memory>
#include <unordered_set>

/*
tcpServer管理所有的TcpConnection
*/

namespace tmms
{
    namespace network
    {
        using NewConnectionCallback = std::function<void (const TcpConnectionPtr &)>;
        using DestroyConnectionCallback = std::function<void (const TcpConnectionPtr&)>;

        class TcpServer
        {
        public:
            TcpServer(EventLoop *loop, const InetAddress &addr); // 运行在哪个loop, 监听的地址
            virtual ~TcpServer();
        
        public:
        // 所有的回调都是为了同时上层的业务层。
            void SetNewConnectionCallback(const NewConnectionCallback &cb);
            void SetNewConnectionCallback(NewConnectionCallback &&cb);
            void SetDestroyConnectionCallback(const DestroyConnectionCallback &cb);
            void SetDestroyConnectionCallback(DestroyConnectionCallback &&cb);
            void SetActiveCallback(const ActiveCallback &cb);
            void SetActiveCallback(ActiveCallback &&cb);
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);
            void SetMessageCallback(const MessageCallback &cb);
            void SetMessageCallback(MessageCallback &&cb);
            void OnAccet(int fd,const InetAddress &addr);
            void OnConnectionClose(const TcpConnectionPtr &con);

            virtual void Start();
            virtual void Stop();

        private:
            EventLoop *loop_{nullptr};
            InetAddress addr_;
            std::shared_ptr<Acceptor> acceptor_;
            NewConnectionCallback new_connection_cb_;
            std::unordered_set<TcpConnectionPtr> connections_;  // 管理所有连接
            MessageCallback message_cb_;
            ActiveCallback active_cb_;
            WriteCompleteCallback write_complete_cb_;
            DestroyConnectionCallback destroy_connection_cb_;
        };
        
    } // namespace network
    
} // namespace tmms
