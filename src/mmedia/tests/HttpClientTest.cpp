/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-04 12:33:23
 * @FilePath: /liveServer/src/mmedia/tests/HttpClientTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "mmedia/http/HttpClient.h"
#include "network/TcpClient.h"
#include "base/FileMgr.h"
#include "base/TaskMgr.h"
#include "live/LiveService.h"
#include "base/TTime.h"
#include "base/Config.h"
#include "base/LogStream.h"


#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;
using namespace tmms::mm;

EventLoopThread eventloop_thread;
std::thread th;

class HttpHandlerImpl : public HttpHandler
{
    public:
        void OnNewConnection(const TcpConnectionPtr &conn) override
        {

        } // 新连接的时候，直播业务就可以处理数据，比如创建用户等
        void OnConnectionDestroy(const TcpConnectionPtr &conn) override
        {

        } // 连接断开的时候，业务层可以回收资源，注销用户等
        void OnRecv(const TcpConnectionPtr& conn, const PacketPtr &data) override
        {
            std::cout << "recv type:" << data->PacketType() << " size:" << data->PacketSize() << std::endl;
        }// 多媒体解析出来的数据，传给直播业务
        void OnRecv(const TcpConnectionPtr& conn, PacketPtr &&data) override
        {
            std::cout << "recv type:" << data->PacketType() << " size:" << data->PacketSize() << std::endl;
        }
        void OnActive(const ConnectionPtr &conn) override
        {

        }   // 新连接回调通知直播业务

        void OnSent(const TcpConnectionPtr &conn)
        {

        }
        bool OnSentNextChunk(const TcpConnectionPtr &conn)
        {
            return false;
        }   
        void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet)
        {
            // 客户端肯定收到的都是响应
            if(!req->IsRequest())
            {
                std::cout << "code:" << req->GetStatusCode() << std::endl;
                if(packet)
                {
                    std::cout  << "body:\n"  << packet->Data()  << std::endl;
                }

            }
        }
};

int main()
{
    g_logger = new Logger(nullptr);
    g_logger->SetLogLevel(KDebug);

    if(!sConfigMgr->LoadConfig("../config/config.json"))
    {
        std::cerr << "load config file failed." << std::endl;
        return -1;
    }
    
    Config::ptr config = sConfigMgr->GetConfig();
    LogInfo::ptr log_info = config->GetLogInfo();
    std::cout << "log level : " << log_info->level 
              << "\nlog path : " << log_info->path
              << "\nlog name : " << log_info->name
              << "\nlog rotate : " << log_info->rotate_type << std::endl;
    FileLog::ptr log = sFileMgr->GetFileLog(log_info->path + log_info->name);
    if(!log)
    {
        std::cerr << "log can't open.exit." << std::endl;
        return -1;
    }
    log->SetRotate(log_info->rotate_type);
    g_logger = new Logger(log);
    g_logger->SetLogLevel(log_info->level);
    
    // 任务
    Task::ptr task = std::make_shared<Task>([](const Task::ptr &task){
        sFileMgr->OnCheck(); //查询切分
        task->Restart();
    }, 1000);
    sTaskMgr->Add(task);
    
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        HttpClient client(loop, new HttpHandlerImpl());
        client.Get("http://www.baidu.com");
        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}