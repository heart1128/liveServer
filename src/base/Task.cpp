/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 16:31:02
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 11:26:38
 * @FilePath: /tmms/src/base/Task.cpp
 * @Description:  learn 
 */
#include "Task.h"
#include "TTime.h"

using namespace tmms::base;

/// @brief  设置定时回调函数
/// @param cb           回调函数
/// @param interval     回调执行间隔
Task::Task(const Task::callback &cb, int64_t interval)
:interval_(interval), when_(TTime::NowMS() + interval), cb_(cb)
{
}

/// @brief 
/// @param cb 
/// @param interval 
Task::Task(const Task::callback &&cb, int64_t interval)
:interval_(interval), when_(TTime::NowMS() + interval), cb_(std::move(cb))
{
}

/// @brief 运行一次任务
void Task::Run()
{
    if(cb_)
    {
        cb_(shared_from_this());
    }
}

/// @brief 重复运行任务，每次运行间隔interval
void Task::Restart()
{
    // 更新时间点
    when_ = interval_ + TTime::NowMS();
}
