/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 21:57:47
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 18:23:11
 * @FilePath: /liveServer/src/live/PlayerUser.cpp
 * @Description:  learn 
 */
#include "PlayerUser.h"

using namespace tmms::live;

PlayerUser::PlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
:User(ptr, stream, s)
{
}

PacketPtr PlayerUser::Meta() const
{
    return meta_;
}
PacketPtr PlayerUser::VideoHeader() const
{
    return video_header_;
}
PacketPtr PlayerUser::AudioHeader() const
{  
    return audio_header_;
}
void PlayerUser::ClearMeta()     // 发送之后清除meta，就知道是已经发送过了 
{
    meta_.reset();
}
void PlayerUser::ClearAudioHeader() 
{
    audio_header_.reset();
}
void PlayerUser::ClearVideoHeader()
{
    video_header_.reset();
}
// 这个函数留给具体的协议用户类发送实现
// rtmp , flv, hls等具 es() = 0;
TimeCorrector& PlayerUser::GetTimeCorrector() // 调整正确的时间戳
{
    return time_corrector_;
}