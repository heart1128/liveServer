/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 14:47:29
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 20:32:18
 * @FilePath: /tmms/src/network/net/EventLoop.cpp
 * @Description:  learn 
 */
#include "EventLoop.h"
#include "network/base/Network.h"
#include <cstring>
#include <fcntl.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

using namespace tmms::network;

static thread_local EventLoop *t_local_eventloop = nullptr; // 每个线程独享自己的eventloop

EventLoop::EventLoop()
:epoll_fd_(::epoll_create(1024)),  // epoll_create的文件描述符数量，建议值
epoll_events_(1024)
{
    // 初始化设置

    if(t_local_eventloop)
    {
        NETWORK_ERROR << "there alread had a eventloop!!!";
        exit(-1);
    }
    
    t_local_eventloop = this;
}

EventLoop::~EventLoop()
{
    Quit();
}

/// @brief 事件循环，一个线程一个epoll_event
void EventLoop::Loop()
{
    looping_ = true;
    int64_t timeout = 1000;

    while(looping_)
    {
        memset(&epoll_events_[0], 0x00, sizeof(struct epoll_event) * epoll_events_.size());
        auto ret = ::epoll_wait(epoll_fd_, 
                                (struct epoll_event*)&epoll_events_[0],
                                static_cast<int>(epoll_events_.size()),
                                timeout);
        
        if(ret >= 0)             // success
        {
            for(int i = 0; i < ret; i++)
            {
                struct epoll_event &ev = epoll_events_[i];
                
                if(ev.data.fd <= 0) // 无效fd
                {
                    continue;
                }
                auto iter = events_.find(ev.data.fd);
                if(iter == events_.end())
                {
                    continue;
                }

                Event::ptr &event = iter->second;

                if(ev.events & EPOLLERR)  // 出错事件
                {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    // 读错误
                    getsockopt(event->Fd(), SOL_SOCKET, SO_ERROR, &error, &len);

                    event->OnError(strerror(error));    // 处理子类处理
                }
                else if((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN)) // 挂起（关闭）事件
                {
                    event->OnClose();
                }
                else if(ev.events & (EPOLLIN | EPOLLPRI)) // 读标志 和tcp带外数据（制定了紧急数据的）的，send加了MSG_OOB标志的数据
                {
                    event->OnRead();
                }
                else if(ev.events & EPOLLOUT)       // 写事件
                {
                    event->OnWrite();
                }
            }

            if(ret == epoll_events_.size())     // 事件吃满了数组，说明并发量很高，需要扩充事件数组
            {
                epoll_events_.resize(epoll_events_.size() * 2);
            }
        }
        else if(ret == 0)       // timeout
        {

        }
        else if(ret < 0)        // error
        {
            NETWORK_ERROR << "epoll wait error.error:" << errno;
        }
    }
}

void EventLoop::Quit()
{
    looping_ = false;
}

void EventLoop::AddEvent(const Event::ptr &event)
{
    // 因为这个事件只在一个线程loop内执行，不用加锁争用

    auto iter = events_.find(event->Fd());
    if(iter != events_.end())       // 存在这个event事件，什么也不做
    {
        return;
    }
    event->event_ |= kEventRead;
    events_[event->Fd()] = event;

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));

    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 添加epoll事件
    if(-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, event->fd_, &ev))
    {
        std::cout << "epoll_ctl_add failed! errno = " << errno << std::endl;
    }
}

void EventLoop::DelEvent(const Event::ptr &event)
{
    auto iter = events_.find(event->Fd());
    if(iter == events_.end())       // 不存在这个event事件，什么也不做
    {
        NETWORK_ERROR << "cant find event fd: " << event->Fd();
        return;
    }

    events_.erase(iter);

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));

    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 删除epoll事件
    if(-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_DEL, event->fd_, &ev))
    {
        std::cout << "epoll_ctl_del failed!" << std::endl;
    }
}

/// @brief 使fd能写，就是添加EPOLLOUT标志
/// @param event  为true，就开启写，为false，原来有写的也会被取消
/// @param enable 
/// @return 使能成功
bool EventLoop::EnableEventWriting(const Event::ptr &event, bool enable)
{
    auto iter = events_.find(event->Fd());
    if(iter == events_.end())               // 不存在报错
    {
        NETWORK_ERROR << "cant find event fd: " << event->Fd();
        return false;
    }

    if(enable) // 开启写
    {
        event->event_ |= kEventWrite;
    }
    else    // 原来有写的就取消
    {
        event->event_ &= ~kEventWrite;
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));

    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 添加epoll事件
    if(-1 == epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev))
    {
        std::cout << "epoll_ctl_mod failed! errno = " << errno << std::endl;
    }
    
    return true;
}

bool EventLoop::EnableEventReading(const Event::ptr &event, bool enable)
{
    auto iter = events_.find(event->Fd());
    if(iter == events_.end())               // 不存在报错
    {
        NETWORK_ERROR << "cant find event fd: " << event->Fd();
        return false;
    }

    if(enable) // 开启读
    {
        event->event_ |= kEventRead;
    }
    else    // 原来有读的，取消掉
    {
        event->event_ &= ~kEventRead;
    }

    struct epoll_event ev;
    memset(&ev, 0x00, sizeof(struct epoll_event));

    ev.events = event->event_;
    ev.data.fd = event->fd_;
    // 添加epoll事件
    epoll_ctl(epoll_fd_, EPOLL_CTL_MOD, event->fd_, &ev);

    return true;
}
