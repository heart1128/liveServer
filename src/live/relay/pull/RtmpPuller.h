/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-18 14:04:37
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-18 15:17:39
 * @FilePath: /liveServer/src/live/relay/pull/RtmpPuller.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "network/net/EventLoop.h"
#include "live/Session.h"
#include "live/relay/pull/Puller.h"
#include "base/Target.h"
#include "mmedia/rtmp/RtmpHandler.h"
#include "mmedia/rtmp/RtmpClient.h"

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;
        using namespace tmms::network;
        using namespace tmms::base;

         /**
          * @description:  使用rtmpClient请求回源数据（然后作为publiser发布），实现RtmpHandler接收数据，实现Puller接口，启动回源。
          *             rtmpCietn是继承tcpCliet的，如果tcpCOnnection有连接，消息都会回调到RtmpClietn进行处理，然后RtmpCLient
          *             回调到RtmpHandler，这里继承RtmpHandler，实现了回调函数进行数据的处理
          *             
          *       
          *         Pull是被外部调用，创建rtmp_client到回源点拉流
          *         OnPlay是在拉流之后，作为publisher给其他点推流
          *             如果是OnRecv就是有数据，把拉流的流数据数据包给调用者（需要回源的）session
         **/
        class RtmpPuller : public Puller, public RtmpHandler
        {
        public:
            RtmpPuller(EventLoop *loop, Session *s, PullHandler *handler);
            ~RtmpPuller();

        public:
        // 启动回源
            bool Pull(const TargetPtr &target);

            void OnNewConnection(const TcpConnectionPtr &conn) override;
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override;
            void OnRecv(const TcpConnectionPtr& conn, PacketPtr &&data) override;
            bool OnPlay(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param) override;

            void OnRecv(const TcpConnectionPtr& conn, const PacketPtr &data) override{}
            void OnActive(const ConnectionPtr &conn)override {}  
        
        private:
            TargetPtr target_;
            RtmpClient * rtmp_client_{nullptr};
        };
        
    } // namespace live
    
} // namespace tmms
