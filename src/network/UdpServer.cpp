#include "UdpServer.h"
#include "network/base/Network.h"

using namespace tmms::network;
UdpServer::UdpServer(EventLoop *loop, InetAddress &server)
:UdpSocket(loop, -1, server, InetAddress()), server_(server)
{
}

UdpServer::~UdpServer()
{
    Stop();
}
/// @brief 也会有多个线程要启动Udp
void UdpServer::Start()
{
    loop_->RunInLoop([this](){
        Open();
    });
}
/// @brief  多线程调用
void UdpServer::Stop()
{
    loop_->RunInLoop([this](){
        loop_->DelEvent(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
        OnClose();  // 关闭套接字
    });
}

/// @brief  进行udp的socket创建
void UdpServer::Open()
{
    // 这里是被start调用的，原则上就应该是在同一个loop中，如果出错了就断言
    loop_->AssertInLoopThread();
    fd_ = SocketOpt::CreateNonblockingUdpSocket(AF_INET);

    //1. 申请失败，关闭
    if(fd_ < 0)
    {
        OnClose();
        return;
    }

    // 2. 申请成功，加入事件循环，绑定，加入EPOLLIN
    loop_->AddEvent(std::dynamic_pointer_cast<UdpServer>(shared_from_this()));
    SocketOpt opt(fd_);
    opt.BindAddress(server_);
}
