/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-08 09:21:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-08 09:38:46
 * @FilePath: /tmms/src/network/net/TcpServer.cpp
 * @Description:  learn 
 */
#include "TcpServer.h"
#include "network/base/Network.h"

using namespace tmms::network;

TcpServer::TcpServer(EventLoop *loop, const InetAddress &addr)
:loop_(loop), addr_(addr)
{
    acceptor_ = std::make_shared<Acceptor>(loop, addr);
}

TcpServer::~TcpServer()
{
}

void TcpServer::SetNewConnectionCallback(const NewConnectionCallback &cb)
{
    new_connection_cb_ = cb;
}

void TcpServer::SetNewConnectionCallback(NewConnectionCallback &&cb)
{
    new_connection_cb_ = std::move(cb);
}

void TcpServer::SetDestroyConnectionCallback(const DestroyConnectionCallback &cb)
{
    destroy_connection_cb_ = cb;
}

void TcpServer::SetDestroyConnectionCallback(DestroyConnectionCallback &&cb)
{
    destroy_connection_cb_ = std::move(cb);
}

void TcpServer::SetActiveCallback(const ActiveCallback &cb)
{
    active_cb_ = cb;
}

void TcpServer::SetActiveCallback(ActiveCallback &&cb)
{
    active_cb_ = std::move(cb);
}

void TcpServer::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

void TcpServer::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}

void TcpServer::SetMessageCallback(const MessageCallback &cb)
{
    message_cb_ = cb;
}

void TcpServer::SetMessageCallback(MessageCallback &&cb)
{
    message_cb_ = std::move(cb);
}

/// @brief 创建TcpConnection，设置回调函数
/// @param fd   服务端fd
/// @param addr     服务端地址
void TcpServer::OnAccet(int fd, const InetAddress &addr)
{
    NETWORK_TRACE << "new connection fd:" << fd << " host:" << addr.ToIpPort();

    TcpConnectionPtr conn = std::make_shared<TcpConnection>(loop_, fd, addr_, addr);
    conn->SetCloseCallback(std::bind(&TcpServer::OnConnectionClose, this, std::placeholders::_1));

    if(write_complete_cb_)
    {
        conn->SetWriteCompleteCallback(write_complete_cb_);
    }

    if(active_cb_)
    {
        conn->SetActiveCallback(active_cb_);
    }

    conn->SetRecvMsgCallback(message_cb_);  // 有消息
    connections_.insert(conn);
    loop_->AddEvent(conn);
    conn->EnableCheckIdleTimeout(30);       // 超时30s
    if(new_connection_cb_)                  // 新连接回调
    {
        new_connection_cb_(conn);
    } 
}

void TcpServer::OnConnectionClose(const TcpConnectionPtr &con)
{
    NETWORK_TRACE << "host:" << con->PeerAddr().ToIpPort() << " closed.";
    loop_->AssertInLoopThread();
    connections_.erase(con);
    loop_->DelEvent(con);
    if(destroy_connection_cb_)
    {
        destroy_connection_cb_(con);
    }
}

/// @brief 开始，只要设置accept的回调函数
void TcpServer::Start()
{
    // 打开监听，有accept事件就进行回调，之后进行connection
    acceptor_->SetAcceptCallback(std::bind(&TcpServer::OnAccet, this, std::placeholders::_1, std::placeholders::_2));
    acceptor_->Start();
}

void TcpServer::Stop()
{
    acceptor_->Stop();
}
