/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:06:40
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 17:18:10
 * @FilePath: /tmms/src/network/net/Acceptor.h
 * @Description:  learn 
 */
#pragma once
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"
#include "network/net/Event.h"
#include "network/net/EventLoop.h"
#include <functional>

namespace tmms
{
    using namespace base;
    namespace network
    {
        using AcceptCallback = std::function<void(int sock, const InetAddress &addr)>;
        
        class Acceptor : public Event
        {
        public:
            Acceptor(EventLoop *loop, const InetAddress &addr);
            ~Acceptor();

            void SetAcceptCallback(AcceptCallback &&cb);
            void SetAcceptCallback(const AcceptCallback &cb);
            void Start();
            void Stop();
            void OnRead() override;
            void OnClose();
            void OnError(const std::string &msg);
        
        private:
            void Open();
            InetAddress addr_;
            AcceptCallback accept_cb_;
            SocketOpt *socket_opt_{nullptr};
        };
    }
} 
