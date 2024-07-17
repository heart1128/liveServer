/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:18:38
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 23:09:53
 * @FilePath: /tmms/src/live/base/TimeCorrector.cpp
 * @Description:  learn 
 */
#include "TimeCorrector.h"
#include "CodecUtils.h"

using namespace tmms::live;

uint32_t TimeCorrector::CorrectTimestamp(const PacketPtr &packet)
{
    // 一个正常的数据包，通常整个pakcet是用flv包装的
    if(!CodecUtils::IsCodecHeader(packet))
    {
        // 包的类型是视频包，不能直接判断包类型
        if(packet->IsVideo())
        {
            return CorrectVideoTimeStampByVideo(packet);
        }
        else if(packet->IsAudio())
        {
            return CorrectAudioTimeStampByAudio(packet);
        }
    }
    return 0;
}

/// @brief 连续音频帧不多的时候
/// @param packet 
/// @return 
uint32_t TimeCorrector::CorrectAudioTimeStampByVideo(const PacketPtr &packet)
{
    ++audio_numbers_between_video_; // 当前是一个音频包
    if(audio_numbers_between_video_ > 1) // 超过1个，至少两个，有连续的音频包
    {
        // 就用音频自己的时间戳
        return CorrectAudioTimeStampByAudio(packet);
    }

    int64_t time = packet->TimeStamp();
    // 如果还没有收到视频包，直接用音频包的时间戳
    // 下一次还没有收到视频包，就走上面的返回值了，因为在视频包里面才会把数量清0
    if(video_original_timestamp_ == -1)
    {
        audio_original_timestamp_ = time;
        audio_corrected_timestamp_ = time;
        return time;
    }

    // 前面有视频包了.
    // 就使用视频的时间戳校正音频的
     // 本次的时间 - 上一次的时间
    int64_t delta = time - video_original_timestamp_;
    // 时间差不在一个范围内，就取默认的
    bool fine = (delta > -kMaxVideoDeltaTime) && (delta < kMaxVideoDeltaTime);
    if(!fine)
    {
        delta = kDefaultVideoDeltaTime;
    }

    // 开始校正视频包时间戳
    audio_original_timestamp_ = time;
    // 这里要用视频的校正时间戳
    audio_corrected_timestamp_ = video_corrected_timestamp_ + delta; // 上次的正确时间 + 时间差=这次正确的时间
    if(audio_corrected_timestamp_ < 0) // 时间戳<0，直接置零
    {
        audio_corrected_timestamp_ = 0;
    }
    return audio_corrected_timestamp_;
}

uint32_t TimeCorrector::CorrectVideoTimeStampByVideo(const PacketPtr &packet)
{
    audio_numbers_between_video_ = 0; // 这是一个新的视频帧，两个视频帧之间的音频数初始化0
    int64_t time = packet->TimeStamp();
    if(video_original_timestamp_ == -1)
    {
        video_original_timestamp_ = time;
        video_corrected_timestamp_ = time;

        // 如果音频包比视频包来的快
        if(audio_original_timestamp_ != -1)
        {
            // 如果视频包和音频包相差的间隔很大
            int32_t delta = audio_original_timestamp_ - video_original_timestamp_;
            if(delta <= -kMaxVideoDeltaTime || delta >= kMaxVideoDeltaTime)
            {
                // 就第一个以音频时间戳为准
                video_original_timestamp_ = audio_original_timestamp_;
                video_corrected_timestamp_ = audio_corrected_timestamp_;
            }
        }
    }

    // 本次的时间 - 上一次的时间
    int64_t delta = time - video_original_timestamp_;
    // 时间差不在一个范围内，就取默认的
    bool fine = (delta > -kMaxVideoDeltaTime) && (delta < kMaxVideoDeltaTime);
    if(!fine)
    {
        delta = kDefaultVideoDeltaTime;
    }

    video_original_timestamp_ = time;
    video_corrected_timestamp_ += delta; // 上次的正确时间 + 时间差=这次正确的时间
    if(video_corrected_timestamp_ < 0) // 时间戳<0，直接置零
    {
        video_corrected_timestamp_ = 0;
    }
    return video_corrected_timestamp_;
}

/// @brief 
/// @param packet 
/// @return 
uint32_t TimeCorrector::CorrectAudioTimeStampByAudio(const PacketPtr &packet)
{
    int64_t time = packet->TimeStamp();

    // 第一个音频包，直接返回，不用修正
    if(audio_original_timestamp_ == -1)
    {
        audio_original_timestamp_ = time;
        audio_corrected_timestamp_ = time;
        return time;
    }
    // 本次的时间 - 上一次的时间
    int64_t delta = time - audio_original_timestamp_;
    // 时间差不在一个范围内，就取默认的
    bool fine = (delta > -kMaxAudioDeltaTime) && (delta < kMaxAudioDeltaTime);
    if(!fine)
    {
        delta = kDefaultAudioDeltaTime;
    }

    audio_original_timestamp_ = time;
    audio_corrected_timestamp_ += delta; // 上次的正确时间 + 时间差=这次正确的时间
    if(audio_corrected_timestamp_ < 0) // 时间戳<0，直接置零
    {
        audio_corrected_timestamp_ = 0;
    }
    return audio_corrected_timestamp_;
}
