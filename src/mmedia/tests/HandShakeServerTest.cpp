
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-10 13:48:29
 * @FilePath: /tmms/src/mmedia/tests/HandShakeServerTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/TcpServer.h"
#include "mmedia/rtmp/RtmpHandShake.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;
using namespace tmms::mm;

EventLoopThread eventloop_thread;
std::thread th;
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

const char *http_response = "HTTP/1.0 200 OK\r\nServer: tmms\r\nContent-Type: text/html\r\nContent-Length: 0\r\n\r\n";
int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        InetAddress listen("10.101.128.69:1935");
        TcpServer server(loop, listen);

        // 设置收到消息回调，直接解析消息的上下文
        server.SetMessageCallback([](const TcpConnectionPtr& con, MsgBuffer &buf){
            RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
            // 每次收到消息就进行回调
            shake->HandShake(buf);
        });

        // 设置有新连接的时候，设置上下文到conn上，设置上下文解析完成之后的回调
        server.SetNewConnectionCallback([&loop](const TcpConnectionPtr &con){
            RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(con, false);
            con->SetContext(kNormalContext, shake);
            // 开始服务端等待C0C1
            shake->Start();
            con->SetWriteCompleteCallback([&loop](const TcpConnectionPtr& con){
                std::cout << "write complete host： " << con->PeerAddr().ToIpPort() << std::endl;
                RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
                // 驱动状态机，每次写完都会启动这个回调
                shake->WriteComplete();
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