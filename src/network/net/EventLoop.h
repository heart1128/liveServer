#pragma once

#include <vector>
#include <sys/epoll.h>
#include <unordered_map>
#include <functional>
#include <queue>
#include <mutex>
#include "Event.h"
#include "PipeEvent.h"
namespace tmms
{
    namespace network
    {
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
            void RunInLoop(const Func &&f);
        
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

        };
    
        
    } // namespace network
    
} // namespace tmms
