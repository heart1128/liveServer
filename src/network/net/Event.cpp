/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 15:08:17
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 16:23:45
 * @FilePath: /tmms/src/network/net/Event.cpp
 * @Description:  learn 
 */
#include "Event.h"
#include "EventLoop.h"
#include <unistd.h>

using namespace tmms::network;

Event::Event()
:fd_(-1), loop_(nullptr)
{
}

Event::Event(EventLoop *loop)
:loop_(loop)
{
}

Event::Event(EventLoop *loop, int fd)
:fd_(fd), loop_(loop)
{
}

Event::~Event()
{
    Close();
}
 
void Event::OnRead()
{
}

void Event::OnWrite()
{
}

void Event::OnClose()
{
}

void Event::OnError(const std::string &msg)
{
}

/// @brief 使能写事件，直接调用的loop
/// @param enable 
/// @return 
bool Event::EnableWriting(bool enable)
{
    return loop_->EnableEventWriting(shared_from_this(),enable);
}

bool Event::EnableReading(bool enable) 
{
    return loop_->EnableEventReading(shared_from_this(),enable);
}

int Event::Fd() const
{
    return fd_;
}

void Event::Close()
{
    if(fd_ > 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
}
