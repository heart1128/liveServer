#include "UdpSocket.h"
#include "network/base/Network.h"

using namespace tmms::network;

// bug: 初始化顺序是按照成员数据声明的顺序初始化的，即使是明显调用了构造函数，顺序还是不变
// 所以这里是message_buffer_先初始化，message_buffer_size_再初始化，但是message_buffer_size_默认为0
// 所以message_buffer_的大小被初始化为0，使用的时候就报错了
UdpSocket::UdpSocket(EventLoop *loop, int sockfd, const InetAddress &localAddr, const InetAddress &peerAddr)
:Connection(loop, sockfd, localAddr, peerAddr), message_buffer_(message_buffer_size_)
{
}

UdpSocket::~UdpSocket()
{
}

void UdpSocket::OnTimeout()
{
    NETWORK_TRACE << "host : " << peer_addr_.ToIpPort() << " timeout.close it";
    OnClose();
}

void UdpSocket::SetCloseCallback(const UdpSocketCloseConnectionCallback &cb)
{
    close_cb_ = cb;
}

void UdpSocket::SetCloseCallback(UdpSocketCloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);
}

void UdpSocket::SetRecvMsgCallback(const UdpSocketMessageCallback &cb)
{
    message_cb_ = cb;
}

void UdpSocket::SetRecvMsgCallback(UdpSocketMessageCallback &&cb)
{
    message_cb_ = std::move(cb);
}

void UdpSocket::SetWriteCompleteCallback(const UdpSocketWriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

void UdpSocket::SetWriteCompleteCallback(UdpSocketWriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}

void UdpSocket::SetTimeoutCallback(int timeout, const UdpSocketTimeoutCallback &cb)
{
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());
        loop_->RunAfter(timeout, [this, cb, us](){
            cb(us);
    });
}

/// @brief 设置timeout的定时任务
/// @param timeout 
/// @param cb 
void UdpSocket::SetTimeoutCallback(int timeout, UdpSocketTimeoutCallback &&cb)
{
    auto us = std::dynamic_pointer_cast<UdpSocket>(shared_from_this());
        loop_->RunAfter(timeout, [this, cb, us](){
            cb(us);
    }); 
}

/// @brief 重新加入时间轮
/// @param max_time 空闲检查时间
void UdpSocket::EvableCheckIdleTimeout(int32_t max_time)
{
    auto tp = std::make_shared<UdpTimeoutEntry>(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
    max_idle_time_ = max_time;
    timeout_entry_ = tp;
    loop_->InsertEntry(max_time, tp);
}

/// @brief 保证在同一个loop中，然后加入写buffer队列就行
/// @param list 
void UdpSocket::Send(std::list<UdpBufferNodePtr> &list)
{
    loop_->RunInLoop([this, &list](){
        SendInLoop(list);
    });
}

void UdpSocket::Send(const char *buf, size_t size, sockaddr *addr, socklen_t len)
{
    loop_->RunInLoop([this, buf, size, addr, len](){
        SendInLoop(buf, size, addr, len);
    });
}

/// @brief 要发送的数据加入到buffer_list中，等待epoll调用OnWrite
/// @param list 
void UdpSocket::SendInLoop(std::list<UdpBufferNodePtr> &list)
{
    for(auto &i : list)
    {
        buffer_list_.emplace_back(i);
    }

    if(!buffer_list_.empty())
    {
        EnableWriting(true);        // fd添加EPOLLOUT
    }
}

void UdpSocket::SendInLoop(const char *buf, size_t size, struct sockaddr *addr, socklen_t len)
{
    if(buffer_list_.empty())    // 为空就是没有任务了，直接写就行，因为只有一个数据写，传入list是有很多的任务
    {
        auto ret = ::sendto(fd_, buf, size, 0, addr, len);
        if(ret > 0) // 写完了返回
        {
            return;
        }
    }
    // list还有数据未处理，只能先加入
    auto node = std::make_shared<UdpBufferNode>(const_cast<char*>(buf), size, addr, len);
    buffer_list_.emplace_back(node);
    EnableWriting(true);
}

void UdpSocket::OnRead()
{
    if(closed_)     // 读之前要判断关闭
    {
        NETWORK_WARN << "host : " << peer_addr_.ToIpPort() << " had closed.";
        return;
    }

    ExtendLife(); // 有读事件，延长生命周期
    while(true)
    {
        struct sockaddr_in6 sock_addr;
        socklen_t len = sizeof(sock_addr);
        // udp的读方式，还接收了发送发的地址
        auto ret = ::recvfrom(fd_, 
                            message_buffer_.BeginWrite(), 
                            message_buffer_size_, 
                            0, 
                            (struct sockaddr*)&sock_addr,
                            &len);
                        
        if(ret > 0) // 缓冲区已读设置
        {
            InetAddress peeraddr;
            message_buffer_.HasWritten(ret);

            if(sock_addr.sin6_family == AF_INET)
            {
                char ip[16] = {0,};
                struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&sock_addr;
                // 网络端的ip转换成host端的
                ::inet_ntop(AF_INET, &(addr_v4->sin_addr.s_addr), ip, sizeof(ip));

                peeraddr.SetAddr(ip);
                peeraddr.SetPort(ntohs(addr_v4->sin_port));
            }
            else if(sock_addr.sin6_family == AF_INET6)
            {
                char ip[INET6_ADDRSTRLEN] = {0,};
                ::inet_ntop(AF_INET6, &(sock_addr.sin6_addr), ip, sizeof(ip));

                peeraddr.SetAddr(ip);
                peeraddr.SetPort(ntohs(sock_addr.sin6_port));
                peeraddr.SetIsIPv6(true);
            }

            // 消息回调
            if(message_cb_)
            {
                message_cb_(peeraddr, message_buffer_);
            }
            // 因为是面向udp数据报的，每次都是一个包的数据处理，不会出现处理完还有数据留在buffer的情况
            message_buffer_.RetrieveAll();
        }
        else if(ret < 0)
        {
            // 资源暂时不可用(EWOULDBLOCK)
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                NETWORK_ERROR << "host : " << peer_addr_.ToIpPort() << " error : " << errno;
                OnClose();
                return;
            }
            break;      // 一定要跳出循环，否则就在这里一直死循环了，因为read是在loop中被调用，是单线程
        }
    }
}

void UdpSocket::OnWrite()
{
    if(closed_)     
    {
        NETWORK_WARN << "host : " << peer_addr_.ToIpPort() << " had closed.";
        return;
    }

    ExtendLife(); // 有写事件，延长生命周期
    while(true)
    {
        // 不为空一直写
        if(!buffer_list_.empty())
        {
            auto buf = buffer_list_.front();
            auto ret = ::sendto(fd_, buf->addr, buf->size, 0, buf->sock_addr, buf->sock_len);
            if(ret > 0)
            {   
                // 一个个删除头，直到为空，list删除O(1)
                buffer_list_.pop_front();
            }
            else if(ret < 0)
            {
                    // 资源暂时不可用(EWOULDBLOCK)
                if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    NETWORK_ERROR << "host : " << peer_addr_.ToIpPort() << " error : " << errno;
                    OnClose();
                    return;
                }
                break;      // 一定要跳出循环，否则就在这里一直死循环了，因为read是在loop中被调用，是单线程
            }
        }

        // 全部写完了，调写完成回调
        if(buffer_list_.empty())
        {
            if(write_complete_cb_)
            {
                write_complete_cb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
            }
        }
    }
}

void UdpSocket::OnClose()
{
    if(!closed_)
    {
        closed_ = true;
        if(closed_)
        {
            close_cb_(std::dynamic_pointer_cast<UdpSocket>(shared_from_this()));
        }
        Event::Close();  // 关闭socket文件描述符
    }
}

void UdpSocket::OnError(const std::string &msg)
{
    NETWORK_TRACE << "host : " << peer_addr_.ToIpPort() << " error:" << msg;
    OnClose();
}

/// @brief  所有线程都会调用ForceClose()，要保证线程安全，在loop这个线程调用的，一定是在loop完成
void UdpSocket::ForceClose()
{
    loop_->RunInLoop([this](){
        OnClose();
    });
}

void UdpSocket::ExtendLife()
{
    auto tp = timeout_entry_.lock();        // 内部指针使用weak_ptr是因为防止这里的引用数量永远为1。
    // 添加时间就是加入一个shared_ptr到时间轮，因为时间轮是指针指针的槽
    // 如果时间轮中的指针引用为0了，就会自动释放，调用析构执行超时回调
    if(tp)
    {
        loop_->InsertEntry(max_idle_time_, tp);
    }
}
