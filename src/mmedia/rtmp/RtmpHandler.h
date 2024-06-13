#pragma once

#include "mmedia/base/MMediaHandler.h"
#include <string>

namespace tmms
{
    namespace mm
    {
        /// @brief  作为扩展功能的实现头
        class RtmpHandler : public MMediaHandler
        {
        public:
            // 播放
            virtual bool OnPlay(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
            // 推流
            virtual bool OnPublish(const TcpConnectionPtr& conn, const std::string &session_name, const std::string &param){return false;}
            // 暂停
            virtual bool OnPause(const TcpConnectionPtr& conn, bool pause){return false;}
            // 定位(快进等)≈
            virtual void OnSeek(const TcpConnectionPtr& conn, double time){}
        };
    } // namespace mm
    
} // namespace tmms
