#include "TcpConnection.h"
#include "network/base/Network.h"
#include <unistd.h>
#include <iostream>

using namespace tmms::network;

TcpConnection::TcpConnection(EventLoop *loop, int socketfd, const InetAddress &localAddr, const InetAddress &peerAddr)
:Connection(loop, socketfd, localAddr, peerAddr)
{
}

TcpConnection::~TcpConnection()
{
    OnClose();
}

void TcpConnection::SetCloseCallback(const CloseConnectionCallback &cb)
{
    close_cb_ = cb;
}

void TcpConnection::SetCloseCallback(CloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);
}

/// @brief 关闭连接，执行关闭回调
void TcpConnection::OnClose()
{
    loop_->AssertInLoopThread();        // 关闭是要判断是在同一个事件循环的
    if(!closed_)
    {
        closed_ = true;     // 这个标志位一定要放在前面，避免多线程重复进来调用close_cb_
        if(close_cb_)
        {
            close_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
        Event::Close();     // 这里直接调用基类的关闭函数，关闭socket 的fd，tcp就会断开连接
    }
}

/// @brief 可能是多线程调用，放在任务队列中执行，保证在同一个事件循环执行
void TcpConnection::ForceClose()
{
    loop_->RunInLoop([this](){
        OnClose();
    });
}

/// @brief 从网络fd中读取到buffer，然后进行回调处理
void TcpConnection::OnRead()
{
    if(closed_) // 关闭不处理
    {
        NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << "had closed!";
        return;
    }
    
    ExtendLife();       // 读一次就应该重新添加进去
    while(true)
    {
        int err = 0;
        auto ret = message_buffer_.ReadFd(fd_, &err);   // 读数据，分散读
        if(ret > 0)
        {
            if(message_cb_)
            {
                message_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()), message_buffer_);
            }
        }
        else if(ret == 0)  // 读完了
        { 
            OnClose();
            break;
        }
        else        //<0出错了
        {
            if(err != EINTR && err != EAGAIN && err != EWOULDBLOCK)
            {
                // NETWORK_ERROR << "read err : " << err;
                OnClose();
            }
            break;  // fixbug:这里没有出错跳出还是在循环读，阻塞在这里了。
        }
    }
}
void TcpConnection::SetRecvMsgCallback(const MessageCallback &cb)
{ 
    message_cb_ = cb;
}
void TcpConnection::SetRecvMsgCallback(MessageCallback &&cb)
{
    message_cb_ = std::move(cb);
}
/// @brief 分散写，不断写入vector的第一个iovec，然后删除头部接着写
void TcpConnection::OnWrite()
{
    if(closed_)
    {
        NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << "had closed.";
        return;
    }

    ExtendLife(); // 有写操作，连接不是无效的
    if(!io_vec_list_.empty())
    {
        while(true) // 不断发
        {
            // 分散写，从一个iovec中写入，里面带多个缓冲区
            auto ret = ::writev(fd_, &io_vec_list_[0], io_vec_list_.size());
            if(ret >= 0)
            {
                while(ret > 0)  // ret是返回写入的长度
                {
                    // 如果第一个数据长度比返回值的大，说明第一个数据还没有写完
                    if(io_vec_list_.front().iov_len > ret)
                    {
                        // iov_base是指针，指向下一次要读取的地方，这里往读取的下一次指向
                        io_vec_list_.front().iov_base = (char*)io_vec_list_.front().iov_base + ret;
                        io_vec_list_.front().iov_len -= ret;
                        break;
                    }
                    else        // 发送完成了，直接清掉第一个
                    {
                        ret -= io_vec_list_.front().iov_len;
                        io_vec_list_.erase(io_vec_list_.begin());
                    }
                }

                if(io_vec_list_.empty()) // 没有数据了，发送完了，不能写了
                {
                    EnableWriting(false);
                    if(write_complete_cb_)
                    {
                        write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
                    }
                    return;
                }
            }
            else  // ret < 0 发送失败
            {
                if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
                {
                    NETWORK_ERROR << "host: " << peer_addr_.ToIpPort() << " write err: " << errno;
                    OnClose();
                    return;
                }
                break;
            }
        } // end while
    } // end if
    else  // 写完了
    {
         EnableWriting(false);
        if(write_complete_cb_)
        {
            write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
        }
        return;
    }
}

void TcpConnection::SetWriteCompleteCallback(const WriteCompleteCallback &cb)
{
    write_complete_cb_ = cb;
}

void TcpConnection::SetWriteCompleteCallback(WriteCompleteCallback &&cb)
{
    write_complete_cb_ = std::move(cb);
}
/// @brief  可能是多个线程同时发，放在同一个loop中
/// @param list 
void TcpConnection::Send(std::list<BufferNode::ptr> &list)
{
    loop_->RunInLoop([this, &list](){
        SendInLoop(list);
    });
}

void TcpConnection::Send(const char *buf, size_t size)
{
    loop_->RunInLoop([this, buf, size](){
        SendInLoop(buf, size);
    });
}

void TcpConnection::OnTimeout()
{
    NETWORK_ERROR << "host: " << peer_addr_.ToIpPort() << " timeout and close it!";
    std::cout << "\nhost: " << peer_addr_.ToIpPort() << " timeout and close it!" << std::endl;
    OnClose();
}

void TcpConnection::SetTimeoutCallback(int timeout, const TimeoutCallback &cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp, &cb]{
        cb(cp);
    });
}

void TcpConnection::SetTimeoutCallback(int timeout, TimeoutCallback &&cb)
{
    auto cp = std::dynamic_pointer_cast<TcpConnection>(shared_from_this());
    loop_->RunAfter(timeout, [&cp, cb]{
        cb(cp);
    });
}
/// @brief 加入超时事件到定时器
/// @param max_time 
void TcpConnection::EnableCheckIdleTimeout(int32_t max_time)
{
    auto tp = std::make_shared<TimeoutEntry>(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
    max_idle_time_ = max_time;
    timeout_entry_ = tp;
    loop_->InsertEntry(max_time, tp);
}

/// @brief 延长超时时间，重新加入到智能指针时间轮
void TcpConnection::ExtendLife()
{
    auto tp = timeout_entry_.lock();        // 内部指针使用weak_ptr是因为防止这里的引用数量永远为1。
    // 添加时间就是加入一个shared_ptr到时间轮，因为时间轮是指针指针的槽
    // 如果时间轮中的指针引用为0了，就会自动释放，调用析构执行超时回调
    if(tp)
    {
        loop_->InsertEntry(max_idle_time_, tp);
    }
}

/// @brief 当前线程的loop才会调用，直接发送，不担心多线程
/// @param buf 
/// @param size 
void TcpConnection::SendInLoop(const char *buf, size_t size)
{
    if(closed_) // 关闭不处理
    {
        NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << "had closed!";
        return;
    }

    size_t send_len = 0;
    if(io_vec_list_.empty())  // 为空就没有任务，可以直接发送，不为空就接着加入到队列中，等待下一次的发送
    {
        send_len = ::write(fd_, buf, size);
        if(send_len < 0)    // 发送数据失败。
        {
            if(errno != EINTR && errno != EAGAIN && errno != EWOULDBLOCK)
            {
                NETWORK_ERROR << "host: " << peer_addr_.ToIpPort() << " write err: " << errno;
                OnClose();
                return;
            }
            send_len = 0;
        }
        size -= send_len; // 已经发送了多少
        if(size == 0)   // 发送完成，调用发送完成的回调
        {
           if(write_complete_cb_)
           {
                write_complete_cb_(std::dynamic_pointer_cast<TcpConnection>(shared_from_this()));
           } 
           return;
        }
    }

    // 发送一次还有数据，加入到队列中等待下一次的发送
    if(size > 0)
    {
        struct iovec vec;
        vec.iov_base = (void*)(buf + send_len);
        vec.iov_len = size;

        io_vec_list_.push_back(vec);
        EnableWriting(true);
    }
}

void TcpConnection::SendInLoop(std::list<BufferNode::ptr> &list)
{
    if(closed_) // 关闭不处理
    {
        NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << "had closed!";
        return;
    }
    // 直接加入到数据队列中
    for(auto &it : list)
    {
        struct iovec vec;
        vec.iov_base = (void*)it->addr;
        vec.iov_len = it->size;

        io_vec_list_.push_back(vec);
    }

    // 有数据就使能写
    if(!io_vec_list_.empty())
    {
        EnableWriting(true);
    }
}


void TcpConnection::OnError(const std::string &msg)
{
    NETWORK_ERROR << "host:" << peer_addr_.ToIpPort() << " error msg:" << msg;
    OnClose();
}


