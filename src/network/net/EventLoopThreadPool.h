#pragma once

#include "base/NonCopyable.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <vector>
#include <atomic>

namespace tmms
{
    namespace network
    {
        class EventLoopThreadPool : public base::NonCopyable
        {
        public:
            using ptr = std::shared_ptr<EventLoopThreadPool>;

            EventLoopThreadPool(int thread_num, int start = 0, int cpus = 4);
            ~EventLoopThreadPool();
        
        public:
            std::vector<EventLoop*> GetLoops() const;
            EventLoop* GetNextLoop();
            size_t Size();
            void Start();
        private:
            std::vector<EventLoopThread::ptr> threads_;  // 线程池
            std::atomic_int32_t loop_index_{0};                     // 当前取到的loop index,多线程并发
        };
        
        
    } // namespace network
    
} // namespace tmms
