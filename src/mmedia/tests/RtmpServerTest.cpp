
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 10:33:06
 * @FilePath: /tmms/src/mmedia/tests/RTMPServerTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/TcpServer.h"
#include "mmedia/rtmp/RtmpHandShake.h"
#include "mmedia/rtmp/RtmpServer.h"

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
        InetAddress listen("10.144.0.1:1935");
        RtmpServer server(loop, listen);
        // 整个Rtmp的回调都设置在里面了，直接启动就行
        // rtmp设置自己的业务流程回调函数到tcpServer中
        // tcpServer设置这个回调到accept和tcpconnection的回调，然后进行调用
        server.Start();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}