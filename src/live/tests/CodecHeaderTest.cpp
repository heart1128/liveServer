/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 16:42:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-28 00:08:44
 * @FilePath: /liveServer/src/live/tests/CodecHeaderTest.cpp
 * @Description:  learn 
 */
#include "network/net/EventLoop.h"
#include "network/net/EventLoopThread.h"
#include "mmedia/rtmp/RtmpClient.h"
#include "network/TcpClient.h"
#include "live/CodecHeader.h"
#include "live/base/CodecUtils.h"

#include <iostream>


using namespace tmms::network;
using namespace tmms::base;
using namespace tmms::mm;
using namespace tmms::live;

EventLoopThread eventloop_thread;
std::thread th;

CodecHeader codec_header;

class RtmpHandlerImpl : public RtmpHandler
{
    public:
        // 播放
        bool OnPlay(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
        // 推流
        bool OnPublish(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
        // 暂停
        void OnPause(const TcpConnectionPtr& conn, bool pause){}
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
            // std::cout << "recv type:" << data->PacketType() << " size:" << data->PacketSize() << std::endl;
        }// 多媒体解析出来的数据，传给直播业务
        void OnRecv(const TcpConnectionPtr& conn, PacketPtr &&data) override
        {
            // std::cout << "recv type:" << data->PacketType() << " size:" << data->PacketSize() << std::endl;
            if(CodecUtils::IsCodecHeader(data))
            {
                codec_header.ParseCodecHeader(data);
            }
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
        client.Play("rtmp://172.27.218.130/live/test");
        while(1)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }

    return 0;
}