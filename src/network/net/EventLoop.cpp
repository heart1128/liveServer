/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 14:47:29
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-17 15:11:28
 * @FilePath: /liveServer/src/network/net/EventLoop.cpp
 * @Description:  learn 
 */
#include "EventLoop.h"
#include "network/base/Network.h"
#include "base/TTime.h"
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
                                timeout);  // 1000ms也就是1s超时，超时不会执行for，只是执行任务队列和触发时间轮
                                
        if(ret >= 0)             // 超时的值是0
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

                /// fixbug: 如果使用else if，因为事件是并发的，触发了一个不再触发别的事件，这个事件就消失了
                if(ev.events & EPOLLERR)  // 出错事件
                {
                    int error = 0;
                    socklen_t len = sizeof(error);
                    // 读错误
                    getsockopt(event->Fd(), SOL_SOCKET, SO_ERROR, &error, &len);

                    event->OnError(strerror(error));    // 处理子类处理
                    continue; // 错了就不能往下了
                }
                if((ev.events & EPOLLHUP) && !(ev.events & EPOLLIN)) // 挂起（关闭）事件
                {
                    event->OnClose();
                    continue;
                }
                if(ev.events & (EPOLLIN | EPOLLPRI)) // 读标志 和tcp带外数据（制定了紧急数据的）的，send加了MSG_OOB标志的数据
                {
                    event->OnRead();
                }
                if(ev.events & EPOLLOUT)       // 写事件
                {
                    event->OnWrite();
                }
            }

            if(ret == epoll_events_.size())     // 事件吃满了数组，说明并发量很高，需要扩充事件数组
            {
                epoll_events_.resize(epoll_events_.size() * 2);
            }

            RunFunctions();     // 无论什么触发了epoll。都会把任务队列的任务全部执行
            int64_t now = tmms::base::TTime::NowMS();
            wheel_.OnTimer(now);
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

    NETWORK_DEBUG << "删除的监听fd = " << event->fd_;

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

/// @brief 判断当前的任务所在线程和eventloop所在是不是同一个线程
void EventLoop::AssertInLoopThread()
{
    if(!IsInLoopThread())
    {
        NETWORK_ERROR << "It is forbidden to run loop on other thread!!!";
        exit(-1);
    }
}

bool EventLoop::IsInLoopThread() const
{
    // 判断当前的线程变量eventloop是不是自己，不是就不在一个线程
    return t_local_eventloop == this;
}

/// @brief 传入任务，如果调用任务的线程和loop是在同一个线程内的，就直接执行，如果不是就加入任务队列
            /// 目的就是：为了保证一个任务是在同一个线程中执行的，而不会因为不在同一个线程中执行，执行到一半
            /// 就被其他线程抢占执行，错乱
            /// 如果是在同一个线程，那么执行就可以了，可以保证执行都是在一个线程内
            /// 如果不是在同一个线程，加入任务队列，loop执行，因为loop是在一个线程执行，所以执行的任务也是在一个线程内执行
/// @param f  执行任务的函数
void EventLoop::RunInLoop(const Func &f)
{
    if(IsInLoopThread())
    {
        f();
    }
    else
    {
        std::lock_guard<std::mutex> lk(lock_);
        functions_.push(f);

        WakeUp();
    }
}

/// @brief 保证回调函数是和loop在一个线程内，是就立刻执行，不是加到任务队列在Loop中执行
/// @param f 
void EventLoop::RunInLoop(Func &&f)
{
    // 这么做的原因是，如果这个任务不是当前线程的，在这里执行了就会切换到其他线程
    // 增加了上下文切换以及线程安全的问题，所以不在这个线程的任务就添加到这个线程执行
    // 不直接执行切换上下文。
    if(IsInLoopThread())
    {
        f();
    }
    else
    {
        // 加入到任务队列，然后执行回调，保证回调在同一个线程不间断执行
        std::lock_guard<std::mutex> lk(lock_);
        functions_.push(std::move(f));

        WakeUp();
    }
}

void EventLoop::InsertEntry(uint32_t delay, EntryPtr entryPtr)
{
    if(IsInLoopThread())
    {
        wheel_.InstertEntry(delay, entryPtr);
    }
    else        // 不在这个线程里就加入任务队列跑
    {
        RunInLoop([this, delay, entryPtr]{    // 同样是插入了时间轮跑
            wheel_.InstertEntry(delay, entryPtr);
        });
    }
}

void EventLoop::RunAfter(double delay, const Func &cb)
{
    if(IsInLoopThread())
    {
        wheel_.RunAfter(delay, cb);
    }
    else        // 不在这个线程里就加入任务队列跑
    {
        RunInLoop([this, delay, cb]{    // 同样是插入了时间轮跑
            wheel_.RunAfter(delay, cb);
        });
    }
}

/// @brief 设置delay秒之后执行cb任务
/// @param delay 单位：s
/// @param cb void()类型的回调函数
void EventLoop::RunAfter(double delay, Func &&cb)
{
    if(IsInLoopThread())
    {
        wheel_.RunAfter(delay, cb);
    }
    else        // 不在这个线程里就加入任务队列跑
    {
        RunInLoop([this, delay, cb]{    // 同样是插入了时间轮跑
            wheel_.RunAfter(delay, cb);
        });
    }
}

void EventLoop::RunEvery(double interval, const Func &cb)
{
    if(IsInLoopThread())
    {
       wheel_.RunEvery(interval, cb);
    }
    else        // 不在这个线程里就加入任务队列跑
    {
        RunInLoop([this, interval, cb]{    // 同样是插入了时间轮跑
           wheel_.RunEvery(interval, cb);
        });
    }  
}

void EventLoop::RunEvery(double interval, Func &&cb)
{
    if(IsInLoopThread())
    {
       wheel_.RunEvery(interval, cb);
    }
    else        // 不在这个线程里就加入任务队列跑
    {
        RunInLoop([this, interval, cb](){    // 同样是插入了时间轮跑
           wheel_.RunEvery(interval, cb);
        });
    }
}

/// @brief 执行任务队列里的任务，因为这个队列是多线程共享，所以要加锁。
void EventLoop::RunFunctions()
{
    std::lock_guard<std::mutex> lk(lock_);
    while(!functions_.empty())
    {
        auto &f = functions_.front();
        f();
        functions_.pop();

        /**
         * bug经历：
         * 
        functions_.pop();
        f();
        这样写出问题了，因为f是引用的，先删除在引用函数就出错了
         * 
        */
    }
}

/// @brief  通过发送管道无用数据，唤醒epoll执行任务队列中人物
void EventLoop::WakeUp()
{
    if(!pipe_event_)
    {
        pipe_event_ = std::make_shared<PipeEvent>(this);
        AddEvent(pipe_event_);      // 添加了管道事件，到时候通过管道写一个数据就行。
    }

    int64_t tmp = 1;        // 读写的数据类型要一样，不然会有数据留在管道，会读阻塞
    pipe_event_->Write((const char*)&tmp, sizeof(tmp));
}
