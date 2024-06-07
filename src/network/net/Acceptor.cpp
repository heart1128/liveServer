/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:14:39
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 17:18:58
 * @FilePath: /tmms/src/network/net/Acceptor.cpp
 * @Description:  learn 
 */
#include "Acceptor.h"
#include "network/base/Network.h"

using namespace tmms::network;

Acceptor::Acceptor(EventLoop *loop, const InetAddress &addr)
:Event(loop), addr_(addr)   // 通过父类去构造继承数据
{
}

Acceptor::~Acceptor()
{
    Stop();
    if(socket_opt_)
    {
        delete socket_opt_;
        socket_opt_ = nullptr;
    }      
}

void Acceptor::SetAcceptCallback(AcceptCallback &&cb)
{
    accept_cb_ = std::move(cb);
}

void Acceptor::SetAcceptCallback(const AcceptCallback &cb)
{
    accept_cb_ = cb;
}

void Acceptor::Start()
{
    loop_->RunInLoop([this](){
        Open();
    });
}

void Acceptor::Stop()
{
    loop_->DelEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
}

/// @brief 重载虚函数，epoll调用的时候执行
void Acceptor::OnRead()
{
    if(!socket_opt_)
    {
        return;
    }
    while(true)
    {
        InetAddress addr;
        auto sock = socket_opt_->Accept(&addr);
        if(sock >= 0)       // accept成功
        {
            if(accept_cb_)  // 执行读回调，服务端读取数据
            {
                accept_cb_(sock, addr);
            }
        }
        else
        {
            // 因为是非阻塞的socket，accept还没有准备好就返回EAGAIN了，这样的情况不是出错
            if(errno != EINTR && errno != EAGAIN)
            {
                NETWORK_ERROR << "acceptor error.errno : " << errno;
                OnClose();
            }
            break;
        }
    }
}

/// @brief 删除epoll的监听事件，然后重新打开accept监听
void Acceptor::OnClose()
{
    Stop();
    Open();
}

void Acceptor::OnError(const std::string &msg)
{
}

/// @brief 创建socket
void Acceptor::Open()
{
    if(fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }

    if(addr_.IsIpV6())
    {
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET6);
    }
    else
    {
        fd_ = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    }

    if(fd_ < 0)
    {
        NETWORK_ERROR << "socket failed.errno : " << errno;
        exit(-1);
    }
    if(socket_opt_)
    {
        delete socket_opt_;
        socket_opt_ = nullptr;
    }

    // 因为模版智能指针只认同类型的，不然无法匹配函数
    // epoll监听accept
    loop_->AddEvent(std::dynamic_pointer_cast<Acceptor>(shared_from_this()));
    socket_opt_ = new SocketOpt(fd_);
    socket_opt_->SetReusePort(true);
    socket_opt_->SetReuseAddr(true);
    socket_opt_->BindAddress(addr_);
    socket_opt_->Listen();
}
