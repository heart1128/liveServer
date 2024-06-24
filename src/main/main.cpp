/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 16:12:24
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 21:15:08
 * @FilePath: /tmms/src/main/main.cpp
 * @Description:  learn 
 */
#include <iostream>
#include <thread>
#include <chrono>
#include "base/FileMgr.h"
#include "base/TaskMgr.h"
#include "base/TTime.h"
#include "base/Config.h"
#include "base/LogStream.h"

using namespace tmms::base;

std::thread t;
// 测试配置文件加载日志切分
void Test_config_and_rotate()
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

int main(int argc, char* argv[])
{
    g_logger = new Logger(nullptr);
    g_logger->SetLogLevel(KTrace);

    if(!sConfigMgr->LoadConfig("../config/config.json"))
    {
        std::cerr << "load config file failed." << std::endl;
        return -1;
    }
    
    Config::ptr config = sConfigMgr->GetConfig();
    LogInfo::ptr log_info = config->GetLogInfo();
    std::cout << "log level : " << log_info->level 
              << "\nlog path : " << log_info->path
              << "\nlog name : " << log_info->name
              << "\nlog rotate : " << log_info->rotate_type << std::endl;
    FileLog::ptr log = sFileMgr->GetFileLog(log_info->path + log_info->name);
    if(!log)
    {
        std::cerr << "log can't open.exit." << std::endl;
        return -1;
    }
    log->SetRotate(log_info->rotate_type);
    g_logger = new Logger(log);
    g_logger->SetLogLevel(log_info->level);
    
    // 任务
    Task::ptr task = std::make_shared<Task>([](const Task::ptr &task){
        sFileMgr->OnCheck(); //查询切分
        task->Restart();
    }, 1000);
    sTaskMgr->Add(task);
    
    Test_config_and_rotate();

    // 启动定时任务
    while(1)
    {
        sTaskMgr->OnWork(); // 50ms轮询检查任务到期
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    return 0;
}