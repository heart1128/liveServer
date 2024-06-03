/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 16:48:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 11:36:35
 * @FilePath: /tmms/src/base/TaskMgr.cpp
 * @Description:  learn
 */
#include "TaskMgr.h"
#include "TTime.h"

using namespace tmms::base;

/// @brief 定时执行任务
void TaskMgr::OnWork()
{
    std::lock_guard<std::mutex> lk(lock_);

    int64_t now = TTime::NowMS();
    // 获取当前时间和任务的执行时间，比较。小于当前时间表示任务超时了，执行
    for(auto iter = tasks_.begin(); iter != tasks_.end();)
    {
        if((*iter)->When() < now)
        {
            (*iter)->Run();
            if((*iter)->When() < now) // 判断有没有重置定时时间，没有就删除
            {
                iter = tasks_.erase(iter); // 删除返回下一个迭代器的地址
                continue;
            }
        }
        ++iter;
    }
}

/// @brief 加入任务
/// @param task  Task类的指针指针
/// @return 是否成功插入
bool TaskMgr::Add(Task::ptr &task)
{
    std::lock_guard<std::mutex> lk(lock_);

    auto iter = tasks_.find(task);
    if(iter != tasks_.end())        // 已经存在了
    {
        return false;
    }
    tasks_.emplace(task);
    return true;
}

/// @brief 
/// @param task 
/// @return 
bool TaskMgr::Del(Task::ptr &task)
{
    std::lock_guard<std::mutex> lk(lock_);
    
    auto iter = tasks_.find(task);
    if(iter != tasks_.end())
    {
        tasks_.erase(iter);
        return true;
    }
    return false;
}
