/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 21:35:35
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 18:22:46
 * @FilePath: /liveServer/src/live/PlayerUser.h
 * @Description:  learn 
 */
#pragma once

#include "User.h"
#include "mmedia/base/Packet.h"
#include "live/base/TimeCorrector.h"
#include "live/Stream.h"
#include <memory>
#include <vector> 

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;
        using namespace tmms::network;

        class Stream;
        using StreamPtr = std::shared_ptr<Stream>;

        class PlayerUser : public User
        {
        public:
            // 方便访问
            friend class Stream;
            // 使用委托构造函数
            // // 表示PlayerUser这个派生类使用User基类的构造函数，自己不使用构造函数
            // using User::User;
            explicit PlayerUser(const ConnectionPtr &ptr,const StreamPtr &stream, const SessionPtr &s);

        public:
            PacketPtr Meta() const;
            PacketPtr VideoHeader() const;
            PacketPtr AudioHeader() const;
            void ClearMeta();     // 发送之后清除meta，就知道是已经发送过了 
            void ClearAudioHeader();
            void ClearVideoHeader();

            // 这个函数留给具体的协议用户类发送实现
            virtual bool PostFrames() = 0;
            // rtmp , flv, hls等具 es() = 0;
            TimeCorrector& GetTimeCorrector(); // 调整正确的时间戳
            
        protected:
            PacketPtr video_header_;
            PacketPtr audio_header_;
            PacketPtr meta_; //  音视频编码信息、时间戳、比特率、帧率等

            bool wait_meta_{true};
            bool wait_audio_{true};
            bool wait_video_{true};

            // 主要判断跳帧之后需不需要重新发送头
            int32_t video_header_index_{0};
            int32_t audio_header_index_{0};
            int32_t meta_index_{0};

            TimeCorrector time_corrector_;
            bool wait_timeout_{false};
            int32_t out_version_{-1}; // 当前输出的版本
            int32_t out_frame_timestamp_{0};
            std::vector<PacketPtr> out_frames_;
            int32_t out_index_{-1}; // 输出序列帧的index
        };

        using PlayerUserPtr = std::shared_ptr<PlayerUser>;
    }
} // namespace tmms
