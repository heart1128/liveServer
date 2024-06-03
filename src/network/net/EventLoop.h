#pragma once

#include <vector>
#include <sys/epoll.h>
#include <unordered_map>
#include "Event.h"

namespace tmms
{
    namespace network
    {
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
        
        private:
            bool looping_{false};   // 是否正在循环
            int epoll_fd_ {-1};
            std::vector<struct epoll_event> epoll_events_;  // epoll事件数组
            std::unordered_map<int, Event::ptr> events_;    // fd和event关联
        };
    
        
    } // namespace network
    
} // namespace tmms
