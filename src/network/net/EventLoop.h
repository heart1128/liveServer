/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 14:45:22
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-22 19:09:17
 * @FilePath: /tmms/src/network/net/EventLoop.h
 * @Description:  learn 
 */
#pragma once

#include <vector>
#include <sys/epoll.h>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>
#include "Event.h"
#include "PipeEvent.h"
#include "TimingWheel.h"
namespace tmms
{
    namespace network
    {
        using EventPtr = std::shared_ptr<Event>;
        using Func = std::function<void()>;
        class EventLoop
        {

        public:
            EventLoop();
            ~EventLoop();

        public:
            void Loop();
            void Quit();
            void AddEvent(const Event::ptr &event);
            void DelEvent(const Event::ptr &event);
            bool EnableEventWriting(const Event::ptr &event, bool enable);
            bool EnableEventReading(const Event::ptr &event, bool enable);

            // 任务队列相关
            void AssertInLoopThread();  // 是否在同一个循环 
            bool IsInLoopThread() const;
            void RunInLoop(const Func& f);
            void RunInLoop(Func &&f);

            // 时间轮
            void InsertEntry(uint32_t delay, EntryPtr entryPtr);   // 插入entry，设置超时时间
            void RunAfter(double delay, const Func &cb);    // 设置延迟多少时间执行
            void RunAfter(double delay, Func &&cb);
            void RunEvery(double interval, const Func &cb);// 每隔一段时间就执行一遍
            void RunEvery(double interval, Func &&cb);
        
        private:
            void RunFunctions();
            void WakeUp();      // 唤醒loop

            bool looping_{false};   // 是否正在循环
            int epoll_fd_ {-1};
            std::vector<struct epoll_event> epoll_events_;  // epoll事件数组
            std::unordered_map<int, Event::ptr> events_;    // fd和event关联

            std::queue<Func> functions_;
            std::mutex lock_;
            PipeEvent::ptr pipe_event_;
            TimingWheel wheel_;
        };
    
        
    } // namespace network
    
} // namespace tmms
