/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-10 13:48:19
 * @FilePath: /tmms/src/mmedia/tests/HandShakeClientTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "mmedia/rtmp/RtmpHandShake.h"
#include "network/TcpClient.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;
using namespace tmms::mm;
using RtmpHandShakePtr = std::shared_ptr<RtmpHandShake>;

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
        InetAddress server("10.101.128.69:1935");
        // 继承了std::enable_shared_from_this<Event>必须使用智能指针
        std::shared_ptr<TcpClient> client = std::make_shared<TcpClient>(loop, server);
        client->SetRecvMsgCallback([](const TcpConnectionPtr& con, MsgBuffer &buf){
            RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
            // 每次收到消息就进行回调
            shake->HandShake(buf);
        });

        client->SetCloseCallback([](const TcpConnectionPtr & con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " closed" << std::endl;
            }
        });
        
        // 写完成要调用
        client->SetWriteCompleteCallback([](const TcpConnectionPtr& con){
            if(con)
            {
                std::cout << "host: "<< con->PeerAddr().ToIpPort() << " write complete" << std::endl;
                RtmpHandShakePtr shake = con->GetContext<RtmpHandShake>(kNormalContext);
                shake->WriteComplete();
            }
        });

        // 建立连接之后，设置上下文
        client->SetConnectCallback([](const TcpConnectionPtr& con, bool connected){
            if(connected)
            {
                RtmpHandShakePtr shake = std::make_shared<RtmpHandShake>(con, true);
                con->SetContext(kNormalContext, shake);
                // 开始客户端发送C0C1
                shake->Start();
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