/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-08 08:55:06
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 16:20:25
 * @FilePath: /tmms/src/network/net/Connection.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "network/net/Event.h"
#include "network/net/EventLoop.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <atomic>

namespace tmms
{
    using namespace base;
    namespace network
    {

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

        enum
        {
            kNormalContext = 0,
            kRtmpContext,       // rtmp
            kHttpContext,       // http
            kUserContext,
            kFlvContext         // flv
        };
        class Connection;
        using ConnectionPtr = std::shared_ptr<Connection>;
        using ContextPtr = std::shared_ptr<void>;
        using ActiveCallback = std::function<void(const ConnectionPtr&)>;       // 连接时候触发的回调函数，返回连接，进行操作

        class Connection : public Event   // connection类作为连接的基类
        {
        public:
            using ptr = std::shared_ptr<Connection>;

            Connection(EventLoop *loop, int fd, const InetAddress &localAddr, const InetAddress &peerAddr);
            virtual ~Connection() = default;
        
        public:
            void SetLoaclAddr(const InetAddress &local);
            void SetPeerAddr(const InetAddress &peer);
            const InetAddress &LocalAddr() const;
            const InetAddress &PeerAddr() const;
            //////// 上下文相关 
            void SetContext(int type, const std::shared_ptr<void> &context);    // 设置上下文
            void SetContext(int type, const std::shared_ptr<void> &&context);    // 设置上下文
            template<typename T> std::shared_ptr<T> GetContext(int type) const; // 获取上下文
            void ClearContext(int type);
            void ClearContext();
            //////// 激活函数相关
            void SetActiveCallback(const ActiveCallback &cb);
            void SetActiveCallback(const ActiveCallback &&cb);
            void Active();
            void Deactive();    
            //////// 关闭时间   子类实现，tcp和udp都要继承连接，关闭的时候做的事不一样
            virtual void ForceClose() = 0;
        
        protected:
            InetAddress local_addr_;
            InetAddress peer_addr_;
        
        private:
            std::unordered_map<int, ContextPtr> contexts_;  //连接上下文管理
            ActiveCallback active_cb_;
            std::atomic<bool> active_;      // 设置激活状态,不同线程调用
        };


        template <typename T>
        inline std::shared_ptr<T> Connection::GetContext(int type) const
        {
            auto iter = contexts_.find(type);
            if(iter == contexts_.end())
            {
                return std::shared_ptr<T>();
            }
            // 同级转换，如果用动态转换就是上下级转换
            return std::static_pointer_cast<T>(iter->second);
        }

    } // namespace network

} // namespace tmms
