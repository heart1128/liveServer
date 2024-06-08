/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-08 10:09:15
 * @FilePath: /tmms/src/network/net/tests/TcpClientTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/net/TcpClient.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;

EventLoopThread eventloop_thread;
std::thread th;

const char *http_request = "GET / HTTP/1.0\r\nHost: 10.101.128.69\r\nAccept: */*\r\nContent-Length: 0\r\n\r\n";
const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        InetAddress server("10.101.128.69:34444");
        // 继承了std::enable_shared_from_this<Event>必须使用智能指针
        std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(loop, server);
        client->SetRecvMsgCallback([](const TcpConnectionPtr& con, MsgBuffer &buf){
            std::cout << "host : " << con->PeerAddr().ToIpPort() << "msg : "<< buf.Peek() << std::endl;
            buf.RetrieveAll();

            con->Send(http_response, strlen(http_response));
        });

        client->SetCloseCallback([](const TcpConnectionPtr & con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " closed" << std::endl;
            }
        });
        
        client->SetWriteCompleteCallback([](const TcpConnectionPtr& con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " write complete" << std::endl;
            }
        });

        client->SetConnectCallback([](const TcpConnectionPtr& con, bool connected){
            if(connected)
            {
                con->Send(http_request, strlen(http_request));
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