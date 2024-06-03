/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 17:00:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 11:26:15
 * @FilePath: /tmms/src/base/Task.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace tmms
{
    namespace base
    {
        class Task;
        

        // 定时任务类，返回自身类的只能指针
        class Task : public std::enable_shared_from_this<Task>
        {
        public:
            using ptr = std::shared_ptr<Task>;
            using callback = std::function<void (const Task::ptr &)>;
        public:
            // 传入的回调函数参数就是Task本身的指针，通过Run执行cb,传入本身，就能在外部控制自己了
            Task(const Task::callback &cb, int64_t interval);
            Task(const Task::callback &&cb, int64_t interval);
        
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
            Task::callback cb_;           // 回调函数
        };
    } // namespace base
} // namespace tmms
