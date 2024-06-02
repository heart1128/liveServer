#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace tmms
{
    namespace base
    {
        class Task;
        using TaskPtr = std::shared_ptr<Task>;
        using TaskCallback = std::function<void (const TaskPtr &)>;

        // 定时任务类，返回自身类的只能指针
        class Task : public std::enable_shared_from_this<Task>
        {
        public:
            // 传入的回调函数参数就是Task本身的指针，通过Run执行cb,传入本身，就能在外部控制自己了
            Task(const TaskCallback &cb, int64_t interval);
            Task(const TaskCallback &&cb, int64_t interval);
        
        public:
            void Run();
            void Restart();
            int64_t When() const
            {
                return when_;
            }
            
        
        private:
            int64_t interval_{0};       // 执行间隔时间
            int64_t when_{0};           // 执行时间点（当前时间 + 传入的间隔）
            TaskCallback cb_;           // 回调函数
        };
    } // namespace base
} // namespace tmms
