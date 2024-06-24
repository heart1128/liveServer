/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:11:08
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 22:18:39
 * @FilePath: /tmms/src/live/base/TimeCorrector.h
 * @Description:  learn
 */
#pragma once

#include "mmedia/base/Packet.h"
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;

        class TimeCorrector
        {
            const int32_t kMaxVideoDeltaTime = 100; // 最大视频间隔，超过这个间隔使用下面默认的
            const int32_t kMaxAudioDeltaTime = 100;
            const int32_t kDefaultVideoDeltaTime = 40; 
            const int32_t kDefaultAudioDeltaTime = 20; 
        public:
            TimeCorrector() = default;
            ~TimeCorrector() = default;
        
        public:
            // 校正时间戳
            uint32_t CorrectTimestamp(const PacketPtr &packet);
            // 没有连续的音频帧，以视频为基准校正（因为直播中视频比音频重要）
            uint32_t CorrectAudioTimeStampByVideo(const PacketPtr &packet);
            // 视频校正
            uint32_t CorrectVideoTimeStampByVideo(const PacketPtr &packet);
            // 在两个视频帧之间有多个音频帧的时候，用音频自己的时间戳校正
            uint32_t CorrectAudioTimeStampByAudio(const PacketPtr &packet);

        private:
            int64_t video_original_timestamp_{-1};  // 源时间戳
            int64_t video_corrected_timestamp_{0};  // 修正后的
            int64_t audio_original_timestamp_{-1};
            int64_t audio_corrected_timestamp_{0};
            int32_t audio_numbers_between_video_{0}; // 两个视频帧之间有多少个音频帧，如果多的话就不依赖视频时间戳修正
        };
        
        
    } // namespace live
    
} // namespace tmms
