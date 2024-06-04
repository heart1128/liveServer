/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 16:41:15
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-04 14:24:28
 * @FilePath: /tmms/src/network/net/EventLoopThread.cpp
 * @Description:  learn 
 */
#include "EventLoopThread.h"

using namespace tmms::network;

EventLoopThread::EventLoopThread()
:thread_([this](){startEventLoop();})       // lambda传入this就能直接用内部函数，开线程
{
}

EventLoopThread::~EventLoopThread()
{
    Run();
    if(loop_)
    {
        loop_->Quit();
    }

    if(thread_.joinable())
    {
        thread_.join();
    }
}

void EventLoopThread::Run()
{
    std::call_once(once_, [this](){
        {
            std::lock_guard<std::mutex> lk(lock_);
            running_ = true;            // 这里赋值true，下面的startEventLoop中的condition_.wait就结束了，进入loop
            condition_.notify_all();    
        }

        auto f = promise_loop_.get_future();    // 返回future
        f.get(); // get在没有set_value的时候进行阻塞，就是要等待loop_ = &loop这步
    });
}

std::thread &EventLoopThread::Thread()
{
    return thread_;
}

EventLoop *EventLoopThread::Loop() const
{
    return loop_;
}

void EventLoopThread::startEventLoop()
{
    EventLoop loop; // 使用一个局部变量的原因是，想让loop的生命周期和线程是一致的

    std::unique_lock<std::mutex> lk(lock_);
    // wait可以传入一个lambda，返回bool，如果为true才会结束wait状态
    condition_.wait(lk, [this](){ return running_;});
    loop_ = &loop;
    promise_loop_.set_value(1); // 解放Run的promise阻塞，目的是为了loop_ = &loop

    loop.Loop();  // 一直循环，等待指令退出 

    loop_ = nullptr; // loop结束，线程结束
}
