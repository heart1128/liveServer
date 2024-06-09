/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 09:04:55
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 14:42:35
 * @FilePath: /tmms/src/network/net/UdpSocket.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "network/base/MsgBuffer.h"
#include "network/net/EventLoop.h"
#include "network/net/Connection.h"
#include <list>
#include <functional>
#include <memory>

namespace tmms
{
    namespace network
    {
        class UdpSocket;
        using UdpSocketPtr = std::shared_ptr<UdpSocket>;
        /// @brief udp服务器需要处理udp包，需要知道那个udp客户端传过来的，tcp是面向连接的。连接之后客户端地址不需要再次识别 
        using UdpSocketMessageCallback = std::function<void (const InetAddress &addr, MsgBuffer &buf)>;
        using UdpSocketCloseConnectionCallback = std::function<void(const UdpSocketPtr&)>;
        using UdpSocketWriteCompleteCallback = std::function<void(const UdpSocketPtr&)>;
        using UdpSocketTimeoutCallback = std::function<void(const UdpSocketPtr&)>;
        struct UdpTimeoutEntry; // 前向声明

        struct UdpBufferNode: public BufferNode  // buffer的存储节点,udp额外加地址，因为sendto要指定地址
        {
            using ptr = std::shared_ptr<BufferNode>;

            UdpBufferNode(void *buf, size_t s, struct sockaddr *addr, socklen_t len)
            :BufferNode(buf, s), sock_addr(addr), sock_len(len)
            {}
            
            struct sockaddr *sock_addr{nullptr};
            socklen_t sock_len{0};
        };

        using UdpBufferNodePtr = std::shared_ptr<UdpBufferNode>;

        class UdpSocket : public Connection
        {
        public:
            using ptr = std::shared_ptr<UdpSocket>;

            UdpSocket(EventLoop *loop, 
                        int sockfd, 
                        const InetAddress &localAddr, 
                        const InetAddress &peerAddr);
            ~UdpSocket();

        public:
            void OnTimeout();
            void SetCloseCallback(const UdpSocketCloseConnectionCallback &cb);
            void SetCloseCallback(UdpSocketCloseConnectionCallback &&cb);
            void SetRecvMsgCallback(const UdpSocketMessageCallback &cb);
            void SetRecvMsgCallback(UdpSocketMessageCallback &&cb);
            void SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb);
            void SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb);
            void SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb);
            void SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb);
            void EvableCheckIdleTimeout(int32_t max_time);

            void Send(std::list<UdpBufferNodePtr>& list);
            void Send(const char* buf, size_t size, struct sockaddr* addr, socklen_t len);
            void SendInLoop(std::list<UdpBufferNodePtr> &list);
            void SendInLoop(const char* buf, size_t size, struct sockaddr* addr, socklen_t len);

            void OnRead() override;
            void OnWrite() override;
            void OnClose() override;
            void OnError(const std::string &msg) override;
            void ForceClose() override;
        
        private:
            void ExtendLife();

            std::list<UdpBufferNodePtr> buffer_list_;
            bool closed_{false};
            int32_t max_idle_time_{30};  // 空闲检测时间，30s
            std::weak_ptr<UdpTimeoutEntry> timeout_entry_; // 因为要加入时间轮，所以用的弱引用，否则时间轮删除全部之后，依旧无法自动释放
            int32_t message_buffer_size_{65535};
            MsgBuffer message_buffer_;
            UdpSocketTimeoutCallback timeout_cb_;
            UdpSocketMessageCallback message_cb_;
            UdpSocketCloseConnectionCallback close_cb_;
            UdpSocketWriteCompleteCallback write_complete_cb_;
        };

        struct UdpTimeoutEntry     // 超时任务监听，智能指针引用结束的时候
        {
        public:
            UdpTimeoutEntry(const UdpSocket::ptr &c)
            :conn(c)
            {
            }
            ~UdpTimeoutEntry()
            {
                auto c = conn.lock();   // 弱引用升级，实现超时回调
                if(c)
                {
                    c->OnTimeout();
                }
            }

        private:
            std::weak_ptr<UdpSocket> conn;
        };
        
           
    } // namespace network
    
} // namespace tmms
