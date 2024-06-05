/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-05 16:52:05
 * @FilePath: /tmms/src/network/net/tests/AcceptorTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;

EventLoopThread eventloop_thread;
std::thread th;

int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        InetAddress server("10.211.55.7:34444");
        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server); 
        acceptor->SetAcceptCallback([](int fd, const InetAddress &addr){
            std::cout << "host : " << addr.ToIpPort() << std::endl;
        });
        acceptor->Start();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}