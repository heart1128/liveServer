/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 14:43:22
 * @FilePath: /tmms/src/network/net/tests/UdpClientTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/UdpClient.h"

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
        InetAddress server("10.101.128.69:34444");
        // 继承了std::enable_shared_from_this<Event>必须使用智能指针
        std::shared_ptr<UdpClient> client = std::make_shared<UdpClient>(loop, server);
        
        client->SetRecvMsgCallback([](const InetAddress &addr, MsgBuffer &buf){
            std::cout << "host : " << addr.ToIpPort() << " \nmsg : "<< buf.Peek() << std::endl;
            buf.RetrieveAll();
        });

        client->SetCloseCallback([](const UdpSocketPtr & con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " closed" << std::endl;
            }
        });
        
        client->SetWriteCompleteCallback([](const UdpSocketPtr& con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " write complete" << std::endl;
            }
        });

        client->SetConnectedCallback([&client](const UdpSocketPtr& con, bool connected){
            if(connected)
            {
                std::cout << "host: " << con->PeerAddr().ToIpPort() << " connected." << std::endl;
                client->Send("1111", strlen("1111"));
            }
        });
        client->Connect();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}