/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 17:16:29
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 21:33:39
 * @FilePath: /tmms/src/network/net/tests/EventLoopThreadTest.cpp
 * @Description:  learn 
 */
#include "network/base/Network.h"
#include "network/net/EventLoopThread.h"
#include "network/net/EventLoopThreadPool.h"
#include "network/net/PipeEvent.h"
#include "base/TTime.h"
#include <iostream>
#include <chrono>

using namespace tmms::network;

EventLoopThread eventLoop_thread;   // 构造器自动创建一个线程
std::thread th;

void TestEventLoopThread()
{
    eventLoop_thread.Run(); // 解除loop前的阻塞，eventloop开始循环
    EventLoop* loop = eventLoop_thread.Loop();


    if(loop)  // 这个loop是内部局部loop的引用，如果内部的eventloop停止了就变nullptr了，实现控制
    {
        std::cout << "loop : " << loop << std::endl;

        PipeEvent::ptr pipeEvent = std::make_shared<PipeEvent>(loop);
        loop->AddEvent(pipeEvent);

        int64_t data = 12345;
        pipeEvent->Write((const char*)&data, sizeof(data));

        th = std::thread([&pipeEvent](){
            while(1)
            {
                int64_t now = tmms::base::TTime::NowMS();
                pipeEvent->Write((const char*)&now, sizeof(now));
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        });

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
}

void TestEventLoopThreadPool()
{
    EventLoopThreadPool pool(2, 0, 2);      // 2个线程，0cpu开始，2个cpu跑

    pool.Start();

    std::vector<EventLoop*> list = pool.GetLoops();

    for(auto &e : list)
    {
        std::cout << "loop : " << e << std::endl; 
    }

    EventLoop* loop = pool.GetNextLoop();
    std::cout << "loop : " << loop << std::endl; 
    loop = pool.GetNextLoop();
    std::cout << "loop : " << loop << std::endl; 
}

int main()
{
    TestEventLoopThreadPool();
    return 0;
}