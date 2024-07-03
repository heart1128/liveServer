/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-29 16:27:09
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-03 11:28:30
 * @FilePath: /liveServer/src/live/LiveService.h
 * @Description:  learn 
 */
#pragma once

#include "network/net/Connection.h"
#include "network/TcpServer.h"
#include "network/net/EventLoopThreadPool.h"
#include "base/TaskMgr.h"
#include "base/NonCopyable.h"
#include "base/Singleton.h"
#include "mmedia/rtmp/RtmpHandler.h"
#include "mmedia/http/HttpHandler.h"
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace tmms
{
    namespace live
    {
        using namespace tmms::network;
        using namespace tmms::base;
        using namespace tmms::mm;

        class Session;
        using SessionPtr = std::shared_ptr<Session>;
        using TaskPtr = std::shared_ptr<Task>;

        class LiveService : public RtmpHandler, public HttpHandler
        {
        public:
            LiveService() = default;
            ~LiveService() = default;
        
        public:
            SessionPtr CreateSession(const std::string &session_name);
            SessionPtr FindSession(const std::string &session_name);
            bool CloseSession(const std::string &session_name); 
            void OnTimer(const TaskPtr &t);
        /// 网络回调
            void OnNewConnection(const TcpConnectionPtr &conn) override;
            void OnConnectionDestroy(const TcpConnectionPtr &conn) override;
            void OnActive(const ConnectionPtr &conn) override;
        /// rtmp回调
            bool OnPlay(const TcpConnectionPtr &conn,const std::string &session_name, const std::string &param) override;
            bool OnPublish(const TcpConnectionPtr &conn,const std::string &session_name, const std::string &param) override;
            void OnRecv(const TcpConnectionPtr &conn ,PacketPtr &&data) override;
            void OnRecv(const TcpConnectionPtr &conn ,const PacketPtr &data) override{};
        /// Http回调
            void OnSent(const TcpConnectionPtr &conn);
            bool OnSentNextChunk(const TcpConnectionPtr &conn);   
            void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet); 
        /// 其他的
            void Start();
            void Stop();
            EventLoop* GetNextLoop();

        private:
            /// @brief 整个直播管理类要管理server，流媒体和用户
            EventLoopThreadPool *pool_{nullptr}; // 返回loop，每种类型的服务都要要添加到每个loop中(rtm，webrtc等等)
            std::vector<TcpServer*> servers_; // 可能有rtmpserver httpserver
            std::mutex lock_;
            std::unordered_map<std::string, SessionPtr> sessions_;
        };
    
        #define sLiveService tmms::base::Singleton<tmms::live::LiveService>::Instance()
    } // namespace live
    
} // namespace tmms
