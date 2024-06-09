/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 16:21:19
 * @FilePath: /tmms/src/network/net/tests/TcpServerTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/TcpServer.h"
#include "network/TestContext.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;

EventLoopThread eventloop_thread;
std::thread th;
using TestContextPtr = std::shared_ptr<TestContext>;

const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        InetAddress listen("10.101.128.69:34444");
        TcpServer server(loop, listen);

        // 设置收到消息回调，直接解析消息的上下文
        server.SetMessageCallback([](const TcpConnectionPtr& con, MsgBuffer &buf){
            TestContextPtr context = con->GetContext<TestContext>(kNormalContext);
            context->ParseMessage(buf);
        });

        // 设置有新连接的时候，设置上下文到conn上，设置上下文解析完成之后的回调
        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con){
            TestContextPtr context = std::make_shared<TestContext>(con);
            con->SetContext(kNormalContext, context);
            context->SetTestMessageCallback([](const TcpConnectionPtr& con, const std::string &msg){
                std::cout << "message : "<< msg << std::endl;
            });
            con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr& con){
                std::cout << "write complete host： " << con->PeerAddr().ToIpPort() << std::endl;
                // con->ForceClose();
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