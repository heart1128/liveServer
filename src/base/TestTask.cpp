/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 17:01:15
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 20:33:24
 * @FilePath: /liveServer/src/base/TestTask.cpp
 * @Description:  learn 
 */
#include "TaskMgr.h"
#include "TTime.h"
#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::base;

void TestTask()
{
    TaskPtr task1 = std::make_shared<Task>([](const TaskPtr &task){
        std::cout << "task1 interval" <<  1000 << "  now:" << TTime::NowMS() << std::endl;
    }, 1000);

    TaskPtr task2 = std::make_shared<Task>([](const TaskPtr &task){
        std::cout << "task2 interval" <<  1000 << "  now:" << TTime::NowMS() << std::endl;
        task->Restart();
    }, 1000);

    TaskPtr task3 = std::make_shared<Task>([](const TaskPtr &task){
        std::cout << "task3 interval" <<  500 << "  now:" << TTime::NowMS() << std::endl;
        task->Restart();
    }, 500);

    TaskPtr task4 = std::make_shared<Task>([](const TaskPtr &task){
        std::cout << "task4 interval" <<  10000 << "  now:" << TTime::NowMS() << std::endl;
    }, 10000);
    
    // 加入定时任务管理
    sTaskMgr->Add(task1);
    sTaskMgr->Add(task2);
    sTaskMgr->Add(task3);
    sTaskMgr->Add(task4);
}

// int main(int argc, char* argv[])
// {
//     TestTask();
//     while(1)
//     {
//         sTaskMgr->OnWork();
//         std::this_thread::sleep_for(std::chrono::microseconds(50));
//     }
//     return 0;
// }