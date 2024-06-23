/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-22 19:50:05
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 15:39:27
 * @FilePath: /tmms/src/mmedia/rtmp/RtmpHandler.h
 * @Description:  learn 
 */
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
            // 告知可以开始publish
            virtual void OnPublishPrepare(const TcpConnectionPtr& conn){}
        };
    } // namespace mm
    
} // namespace tmms
