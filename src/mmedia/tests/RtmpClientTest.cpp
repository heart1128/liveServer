/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 13:50:29
 * @FilePath: /tmms/src/mmedia/tests/RtmpClientTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "network/base/InetAddress.h"
#include "mmedia/rtmp/RtmpClient.h"
#include "network/TcpClient.h"

#include <iostream>
#include <thread>
#include <chrono>

using namespace tmms::network;
using namespace tmms::base;
using namespace tmms::mm;

EventLoopThread eventloop_thread;
std::thread th;

class RtmpHandlerImpl : public RtmpHandler
{
    public:
        // 播放
        bool OnPlay(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
        // 推流
        bool OnPublish(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
        // 暂停
        bool OnPause(const TcpConnectionPtr& conn, bool pause){return false;}
        // 定位(快进等)≈
        void OnSeek(const TcpConnectionPtr& conn, double time){}

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
};

int main()
{
    eventloop_thread.Run();
    EventLoop* loop = eventloop_thread.Loop();

    if(loop)
    {
        RtmpClient client(loop, new RtmpHandlerImpl());
        client.Play("rtmp://10.144.0.1/live/test");
        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}