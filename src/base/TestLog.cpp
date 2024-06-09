/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 20:31:29
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 15:35:34
 * @FilePath: /tmms/src/base/TestLog.cpp
 * @Description:  learn 
 */
#include "LogStream.h"
#include "Logger.h"
#include "FileLog.h"
#include "FileMgr.h"
#include "TTime.h"
#include "Task.h"
#include "TaskMgr.h"
#include <thread>
#include <chrono>

using namespace tmms::base;

std::thread t;

void TestLog()
{
    t = std::thread([](){
        while(true)
        {
            LOG_DEBUG << "test debug !! now : " << TTime::NowMS();
            LOG_INFO << "test info !! now : "<< TTime::NowMS();

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
    });
}

// int main()
// {
//     FileLog::ptr log = sFileMgr->GetFileLog("test.log");
//     log->SetRotate(kRotateMinute);
//     tmms::base::g_logger = new Logger(log);
//     tmms::base::g_logger->SetLogLevel(KTrace);

//     // 注册定时任务
//     Task::ptr task = std::make_shared<Task>([](const Task::ptr &task){
//         sFileMgr->OnCheck();    // 检测文件切分
//         task->Restart(); 
//     }, 1000);

//     sTaskMgr->Add(task);

//     TestLog();

//     while(1)
//     {
//         sTaskMgr->OnWork();
//         std::this_thread::sleep_for(std::chrono::microseconds(50));
//     }
//     return 0;
// }