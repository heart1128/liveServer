/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 15:03:15
 * @FilePath: /tmms/src/network/net/tests/UdpServerTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/UdpServer.h"

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
        InetAddress listen("10.101.128.69:34444");
        // 继承了std::enable_shared_from_this<Event>必须使用智能指针
        std::shared_ptr<UdpServer> server = std::make_shared<UdpServer>(loop, listen);
        
        // 测试OnRead，收到什么发回什么,echo服务器
        server->SetRecvMsgCallback([&server](const InetAddress &addr, MsgBuffer &buf){
            std::cout << "host : " << addr.ToIpPort() << " \nmsg : "<< buf.Peek() << std::endl;

            struct sockaddr_in6 saddr;
            socklen_t len = sizeof(saddr);
            addr.GetSockAddr((struct sockaddr*)&saddr);
            server->Send(buf.Peek(), buf.ReadableBytes(), (struct sockaddr*)&saddr, len);

            buf.RetrieveAll();
        });

        server->SetCloseCallback([](const UdpSocketPtr & con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " closed" << std::endl;
            }
        });
        
        server->SetWriteCompleteCallback([](const UdpSocketPtr& con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " write complete" << std::endl;
            }
        });
        
        server->Start();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}