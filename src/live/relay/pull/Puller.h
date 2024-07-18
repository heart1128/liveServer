/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-17 17:13:05
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-18 14:00:39
 * @FilePath: /liveServer/src/live/relay/pull/Puller.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "network/net/EventLoop.h"
#include "live/Session.h"
#include "base/AppInfo.h"
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace tmms::network;
        class PullHandler
        {
        public:
            PullHandler(){}
            virtual ~PullHandler(){}
            virtual void OnPullSucess() = 0;
            virtual void OnPullClose() = 0;
        };

        class Puller
        {
        public:
            Puller(EventLoop *event_loop, Session *s, PullHandler *pull_handler)
            :event_loop_(event_loop), session_(*s), pull_handler_(pull_handler){}
            virtual ~Puller(){}

            virtual bool Pull(TargetPtr &target) = 0;
            EventLoop *GetEventLoop() const
            {
                return event_loop_;
            }

        
        protected:
            EventLoop *event_loop_{nullptr};
            Session &session_;
            TargetPtr targer_;
            PullHandler *pull_handler_{nullptr};
        };
        
    } // namespace live
    
} // namespace tmms
