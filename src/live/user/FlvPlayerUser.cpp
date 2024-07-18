/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-29 12:47:20
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-06 13:16:18
 * @FilePath: /liveServer/src/live/FlvPlayerUser.cpp
 * @Description:  learn 
 */
#include "FlvPlayerUser.h"
#include "live/Stream.h"
#include "mmedia/flv/FlvContext.h"
#include "live/base/LiveLog.h"
#include "base/TTime.h"

using namespace tmms::live;

using FlvContextPtr = std::shared_ptr<FlvContext>;

FlvPlayerUser::FlvPlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
:PlayerUser(ptr, stream, s)
{
}

/// @brief 发送多个帧 , 包括头
/// @return
bool FlvPlayerUser::PostFrames()
{
    // 流没有准备好或者元数据都没有发送
    if(!stream_->Ready() || ! stream_->HasMedia())
    {
        Deactive(); // 用户休眠，就是设置为连接未激活状态
        return false;
    }

    if(!http_header_send)
    {
        PushFlvHttpHeader();
        return false;
    }
    stream_->GetFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
    // 发送元数据  
    if(meta_)
    {
        auto ret = PushFrame(meta_, true);
        if(ret)
        {
            LIVE_INFO << "flv sent meta now: " << tmms::base::TTime::NowMS()
                    << " host:" << user_id_;
            
            meta_.reset();
        }
    }
    else if(audio_header_)
    {
        auto ret = PushFrame(audio_header_, true);
        if(ret)
        {
            LIVE_INFO << "flv sent audio_header_ now: " << tmms::base::TTime::NowMS()
                    << " host:" << user_id_;
            
            audio_header_.reset();
        }
    }
    else if(video_header_)
    {
        auto ret = PushFrame(video_header_, true);
        if(ret)
        {
            LIVE_INFO << "flv sent video_header_ now: " << tmms::base::TTime::NowMS()
                    << " host:" << user_id_;
            
            video_header_.reset();
        }
    }
    else if(!out_frames_.empty()) // 发送音视频数据
    {
        auto ret = PushFrame(out_frames_);
        if(ret)
        {
            out_frames_.clear();
        }
    }
    else
    {
        // 没有数据了
        Deactive();
    }
    return true;
}

/// @brief 每个类型客户的类型是固定的
/// @return 
UserType FlvPlayerUser::GetUserType() const
{
    return UserType::kUserTypePlayerFlv;
}

/// @brief 发送单帧数据
/// @param packet 
/// @param is_header 
/// @return 
bool FlvPlayerUser::PushFrame(PacketPtr &packet, bool is_header)
{
    auto cx = connection_->GetContext<FlvContext>(kFlvContext);
    if(!cx || cx->Ready())
    {
        return false;
    }

    int64_t ts = 0;
    // 不是头修正时间戳
    if(!is_header) 
    {
        ts = time_corrector_.CorrectTimestamp(packet);
    }

    // 如果是头，就是fmt0，是第一个包
    // 构建flv包使用flv协议发送
    cx->BuildFlvFrame(packet, ts);
    cx->Send();
    return true;
}

bool FlvPlayerUser::PushFrame(std::vector<PacketPtr> &list)
{
    auto cx = connection_->GetContext<FlvContext>(kFlvContext);
    if(!cx || cx->Ready())
    {
        return false;
    }

    int64_t ts = 0;
    for(int i = 0; i < list.size(); ++i)
    {
        PacketPtr &packet = list[i];
        ts = time_corrector_.CorrectTimestamp(packet);
        cx->BuildFlvFrame(packet, ts);
    }
    // 发送整个缓冲区
    cx->Send();
    return true;
}

void FlvPlayerUser::PushFlvHttpHeader()
{
    auto cxt = connection_->GetContext<FlvContext>(kFlvContext);
    if(cxt)
    {
        bool has_video = stream_->HasVideo();
        bool has_audio = stream_->HasAudio();
        cxt->SendHttpHeader(has_video, has_audio);
        http_header_send = true;
    }
}
