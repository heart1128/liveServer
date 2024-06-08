/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-08 09:50:32
 * @FilePath: /tmms/src/network/net/tests/TcpServerTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/net/TcpServer.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;

EventLoopThread eventloop_thread;
std::thread th;

const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        InetAddress listen("10.101.128.69:34444");
        TcpServer server(loop, listen);
        server.SetMessageCallback([](const TcpConnectionPtr& con, MsgBuffer &buf){
            std::cout << "host : " << con->PeerAddr().ToIpPort() << "msg : "<< buf.Peek() << std::endl;
            buf.RetrieveAll();

            con->Send(http_response, strlen(http_response));
        });

        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con){
            con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr& con){
                std::cout << "write complete host ： " << con->PeerAddr().ToIpPort() << std::endl;
                con->ForceClose();
            });
        });

        server.Start();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}