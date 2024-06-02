#pragma once
#include "Task.h"
#include "NonCopyable.h"
#include "Singleton.h"
#include <unordered_set>
#include <mutex>

namespace tmms
{
    namespace base
    {
        class TaskMgr : public NonCopyable      // 基本管理类都是单例
        {
        public:
            TaskMgr() = default;
            ~TaskMgr() = default;
        
        public:
            void OnWork();
            bool Add(TaskPtr &task);
            bool Del(TaskPtr &task);

        private:
            std::unordered_set<TaskPtr> tasks_; // 直接遍历使用hash
            std::mutex lock_;
        };
        // 方便单例访问
        #define sTaskMgr tmms::base::Singleton<tmms::base::TaskMgr>::Instance()
    } 
} // namespace tmms
