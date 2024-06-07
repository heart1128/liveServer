/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-07 21:43:56
 * @FilePath: /tmms/src/network/net/tests/TcpConnectionTest.cpp
 * @Description:  learn 
 */
#include "network/net/Acceptor.h"
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "network/net/TcpConnection.h"

#include "base/FileMgr.h"
#include "base/TaskMgr.h"
#include "base/TTime.h"
#include "base/Config.h"
#include "base/LogStream.h"

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
// {
//     if(!sConfigMgr->LoadConfig("../config/config.json"))
//     {
//         std::cerr << "load config file failed." << std::endl;
//         return -1;
//     }
    
//     Config::ptr config = sConfigMgr->GetConfig();
//     LogInfo::ptr log_info = config->GetLogInfo();
//     FileLog::ptr log = sFileMgr->GetFileLog(log_info->path + log_info->name);
//     if(!log)
//     {
//         std::cerr << "log can't open.exit." << std::endl;
//         return -1;
//     }
//     log->SetRotate(log_info->rotate_type);
//     g_logger = new Logger(log);
//     g_logger->SetLogLevel(log_info->level);

    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        std::vector<TcpConnectionPtr> list;

        InetAddress server("10.211.55.7:34444");
        std::shared_ptr<Acceptor> acceptor = std::make_shared<Acceptor>(loop, server); 
        // accept回调，每个accept都创建一个tcpconnetion
        acceptor->SetAcceptCallback([&loop, &server, &list](int fd, const InetAddress &addr){
            std::cout << "host : " << addr.ToIpPort() << std::endl;
            TcpConnectionPtr connection = std::make_shared<TcpConnection>(loop, fd, server, addr);
            // 收到消息的回调
            connection->SetRecvMsgCallback([](const TcpConnectionPtr& conn, MsgBuffer& buffer){
                std::cout << "recv msg: " << buffer.Peek() << std::endl;
                buffer.RetrieveAll();
                conn->Send(http_response, strlen(http_response));
            });
            // 写完成回调
            connection->SetWriteCompleteCallback([&loop](const TcpConnectionPtr& conn){
                std::cout << "write complete host : " << conn->PeerAddr().ToIpPort() << std::endl;
                loop->DelEvent(conn);
                conn->ForceClose();
            });

            // 测试超时事件
            connection->EnableCheckIdleTimeout(0.5);
            list.push_back(connection);
            loop->AddEvent(connection); // 加入到事件循环中监听，epoll调用OnRead，调用回调
        });
        acceptor->Start();

        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}