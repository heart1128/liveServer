/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 16:41:08
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 21:22:18
 * @FilePath: /tmms/src/network/net/EventLoopThread.h
 * @Description:  learn 
 */
#pragma once

#include "base/NonCopyable.h"
#include "EventLoop.h"
#include <thread>
#include <mutex>
#include <future>
#include <condition_variable>

namespace tmms
{
    namespace network
    {
        class EventLoopThread : public base::NonCopyable
        {
        public:
            using ptr = std::shared_ptr<EventLoopThread>;
        public:
            EventLoopThread(/* args */);
            ~EventLoopThread();
        
        public:
            void Run();
            std::thread &Thread();
            EventLoop* Loop() const;
        
        private:
            void startEventLoop();

            EventLoop* loop_{nullptr};
            std::thread thread_;
            bool running_{false};
            std::mutex lock_;
            std::condition_variable condition_;
            std::once_flag once_;   // 只运行一次
            std::promise<int> promise_loop_;
        };

           
    } // namespace network
    
} // namespace tmms
