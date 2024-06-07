/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-07 09:40:47
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 09:57:30
 * @FilePath: /tmms/src/network/net/Connection.cpp
 * @Description:  learn 
 */
#include "Connection.h"

using namespace tmms::network;
using namespace tmms::base;

Connection::Connection(EventLoop *loop, int fd, const InetAddress &localAddr, const InetAddress &peerAddr)
:Event(loop, fd), local_addr_(localAddr), peer_addr_(peerAddr)
{

}

void Connection::SetLoaclAddr(const InetAddress &local)
{
    local_addr_ = local;
}

void Connection::SetPeerAddr(const InetAddress &peer)
{
    peer_addr_ = peer;
}

const InetAddress &Connection::LocalAddr() const
{
    return local_addr_;
}

const InetAddress &Connection::PeerAddr() const
{
    return peer_addr_;
}

void Connection::SetContext(int type, const std::shared_ptr<void> &context)
{
    contexts_[type] = context;
}

void Connection::SetContext(int type, const std::shared_ptr<void> &&context)
{
    contexts_[type] = std::move(context);
}

void Connection::ClearContext(int type)
{
    contexts_[type].reset();
}

void Connection::ClearContext()
{
    contexts_.clear();
}

void Connection::SetActiveCallback(const ActiveCallback &cb)
{
    active_cb_ = cb;
}

void Connection::SetActiveCallback(const ActiveCallback &&cb)
{
    active_cb_ = std::move(cb);
}

/// @brief 事件激活，需要在EventLoop中激活
void Connection::Active()
{
    if(!active_.load()) 
    {
        loop_->RunInLoop([this](){     // 放在loop中进行回调执行，OnRead()中执行这个回调
            active_.store(true);
            if(active_cb_)  // 回调函数的激活
            {
                // 因为智能指针参数是严格类型比较的，所以要进行转换
                active_cb_(std::dynamic_pointer_cast<Connection>(shared_from_this()));
            }
        });
    }

}

void Connection::Deactive()
{
    active_.store(false);
}
