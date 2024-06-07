/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-07 10:01:01
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 17:19:52
 * @FilePath: /tmms/src/network/net/TcpConnection.h
 * @Description:  learn 
 */
#pragma once
#include "Connection.h"
#include "network/base/InetAddress.h"
#include "network/base/MsgBuffer.h"
#include <functional>
#include <memory>
#include <list>
#include <sys/uio.h>

namespace tmms
{
    using namespace base;
    namespace network
    {
        class TimeoutEntry;
        class TcpConnection;
        using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
        using CloseConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
        using MessageCallback = std::function<void(const TcpConnectionPtr&, MsgBuffer&)>;
        using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
        using TimeoutCallback = std::function<void(const TcpConnectionPtr&)>;
        

        struct BufferNode   // buffer的存储节点
        {
            using ptr = std::shared_ptr<BufferNode>;

            BufferNode(void *buf, size_t s)
            :addr(buf), size(s)
            {}
            void *addr {nullptr};
            size_t size {0};
        };

        using BufferNodePtr = std::shared_ptr<BufferNode>;

        class TcpConnection : public Connection
        {
        public:
            using ptr = std::shared_ptr<TcpConnection>;
            TcpConnection(EventLoop *loop, int socketfd, const InetAddress &localAddr, const InetAddress &peerAddr);
            ~TcpConnection();
        
        public:
            ////// 事件
                // 关闭事件
            void SetCloseCallback(const CloseConnectionCallback &cb);
            void SetCloseCallback(CloseConnectionCallback &&cb);
            void OnClose() override;
            void ForceClose() override;
                // 读事件
            void OnRead() override;
            void SetRecvMsgCallback(const MessageCallback& cb);
            void SetRecvMsgCallback(MessageCallback&& cb);
                // 写事件
            void OnWrite() override;
            void SetWriteCompleteCallback(const WriteCompleteCallback &cb);
            void SetWriteCompleteCallback(WriteCompleteCallback &&cb);
            void Send(std::list<BufferNodePtr> &list);        // 传多个buffer
            void Send(const char* buf, size_t size);            // 发送单个buffer
                // 超时事件
            void OnTimeout();
            void SetTimeoutCallback(int timeout, const TimeoutCallback &cb);
            void SetTimeoutCallback(int timeout,TimeoutCallback &&cb);
            void EnableCheckIdleTimeout(int32_t max_time);

            void OnError(const std::string &msg) override;
            
        
        private:
            void SendInLoop(const char *buf, size_t size);
            void SendInLoop(std::list<BufferNode::ptr> &list);

            void ExtendLife();  // 延长timeout的生命周期

            bool closed_{false};
            CloseConnectionCallback close_cb_;
            MsgBuffer message_buffer_;
            MessageCallback message_cb_;  // 参数表示，哪个连接，使用的哪个buffer

            std::vector<struct iovec> io_vec_list_;
            WriteCompleteCallback write_complete_cb_;

            std::weak_ptr<TimeoutEntry> timeout_entry_; // 弱引用，防止循环引用
            int32_t max_idle_time_{30};     // 可以通过配置文件配置
        };

        struct TimeoutEntry     // 超时任务监听，智能指针引用结束的时候
        {
        public:
            TimeoutEntry(const TcpConnectionPtr &c)
            :conn(c)
            {
            }
            ~TimeoutEntry()
            {
                auto c = conn.lock();   // 弱引用升级，实现超时回调
                if(c)
                {
                    c->OnTimeout();
                }
            }

        private:
            std::weak_ptr<TcpConnection> conn;
        };
        
    } // namespace network
    
} // namespace tmms
