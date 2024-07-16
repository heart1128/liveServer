/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 22:32:21
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-06 12:06:53
 * @FilePath: /liveServer/src/live/Stream.cpp
 * @Description:  learn 
 */
#include "Stream.h"
#include "base/TTime.h"
#include "live/base/CodecUtils.h"
#include "live/base/LiveLog.h"
#include "Session.h"

using namespace tmms::live;
using namespace tmms::base;

Stream::Stream(const std::string &session_name, Session &s)
:session_name_(session_name), session_(s), packet_buffer_(packet_buffer_size_)
{
    stream_time_.store(TTime::NowMS());
    start_timestamp_ = TTime::NowMS();
}

int64_t Stream::ReadyTime() const
{
    return ready_time_;
}

/// @brief 创建流到现在的时间
/// @return 
int64_t Stream::SinceStart() const
{
    return TTime::NowMS() - start_timestamp_;
}

/// @brief 判断有没有流超时,超时时间暂时设置为20s，要从会话配置取
/// @return 
bool Stream::Timeout()
{
    // fixbug: stream_time_只是在构造中赋值了，没有更新过，20s之后一定超时
    // 应该是在每次都有流数据的时候同步更新
    auto delta = TTime::NowMS() - stream_time_.load();
    if(delta > 20 * 1000)
    {
        return true;
    }
    return false;
}

/// @brief 获取到第一帧数据的时间
/// @return 
int64_t Stream::DataTime() const
{
    return data_comming_time_;
}

const std::string &Stream::SessionName() const
{
    return session_name_;
}

int32_t Stream::StreamVersion() const
{
    return stream_version_;
}

bool Stream::HasMedia() const
{
    return has_meta_;
}

bool Stream::Ready() const
{
    return ready_;
}

void Stream::SetReady(bool ready)
{
    ready_ = ready;
    ready_time_ = TTime::NowMS();
}


/// @brief 从推流中获取数据包
/// @param packet 
void Stream::AddPacket(PacketPtr &&packet)
{
    // 1. 用户可能卡顿之类的，就要校正时间戳
    auto t = time_corrector_.CorrectTimestamp(packet);
    packet->SetTimeStamp(t);

    // 有数据包，要更新stream_time_
    stream_time_.store(TTime::NowMS());

    // 里面是要访问has_video_等多线程变量的，所以加锁
    {
        std::lock_guard<std::mutex> lk(lock_);
        // 一个包一帧，计算第几帧
        auto index = ++frame_index_;
        packet->SetIndex(index);
        // 这里判断了是视频和关键帧
        if(packet->IsVideo() && CodecUtils::IsKeyFrame(packet))
        {
            SetReady(true); // 收到关键帧了
            packet->SetPacketType(kPacketTypeVideo | kPacketTypeKeyFrame);
        }

        // h264格式的视频包，放在flv的body部分
        // AVC（Advanced Video Coding）包类型定义了FLV标签中的H.264视频数据的具体类型
        // 首先是avc，可以判断是不是视频头（携带视频的一些信息）
        if(CodecUtils::IsCodecHeader(packet))
        {
            
            codec_headers_.ParseCodecHeader(packet);
            if(packet->IsVideo())
            {
                has_video_ = true;
                stream_version_++;
            }
            else if(packet->IsAudio())
            {
                has_audio_ = true;
                stream_version_++;
            }
            else if(packet->IsMeta()) // rtmp的meta数据也是flv封装的
            {
                has_meta_ = true;
                stream_version_++;
            }
        }

        // 增加帧，其中只有到了关键帧才会添加进gop容器，其他的都是累计长度
        gop_mgr_.AddFrame(packet);
        ProcessMpegts(packet); // 拿到flv数据，处理成mpegts格式给hls使用
        packet_buffer_[index % packet_buffer_size_] = std::move(packet);
        // 超过最大帧大小，就清理gop容器
        auto min_idx = frame_index_ - packet_buffer_size_;
        if(min_idx > 0)
        {
            gop_mgr_.ClearExpriedGop(min_idx);
        }
    }


    // 下面不能加锁，加锁就重入了，因为有调用关系
    // 之前没有设置的第一个数据时间
    if(data_comming_time_ == 0)
    {
        data_comming_time_ = TTime::NowMS();
    }

    start_timestamp_ = TTime::NowMS();
    auto frame = frame_index_.load();
    // 前300帧发快点，后面的是5帧一次
    if(frame < 300 || frame % 5 == 0)
    {
        session_.ActiveAllPlayers();
    }
}

/// @brief user保存要播放的帧，这里是按照流程寻找下一帧的播放
/// @param user 
void Stream::GetFrames(const PlayerUserPtr &user)
{
    // 序列头就是meta数据
    // 没有序列头不能发数据
    if(!HasMedia())
    {
        return;
    }
    // 如果前面的meta或者头，还有数据帧还没有发送完，就不能发送，否则数据堆积了
    if(user->meta_
    || user->audio_header_
    || user->video_header_
    || !user->out_frames_.empty())
    {
        return;
    }

    std::lock_guard<std::mutex> lk(lock_);
    // 如果有输出的帧了>=0，就要判断音视频帧是否落后太多
    if(user->out_index_ >= 0)
    {
        // 当前帧idx - 
        int min_idx = frame_index_ - packet_buffer_size_;
        // 配置文件里配置的lantency, timestamp落后很多也不行
        int content_lantency = user->GetAppInfo()->content_latency;
        if(user->out_index_ < min_idx
            || (gop_mgr_.LastestTimeStamp() - user->out_frame_timestamp_) > 2 * content_lantency)
        {
            LIVE_INFO << "need skip out index:" << user->out_index_
                        << ", min idx:" << min_idx
                        << ", out timestamp:" << user->out_frame_timestamp_
                        << ", latest timestamp:" << gop_mgr_.LastestTimeStamp();
            // 跳帧
            SkipFrame(user);
        }
    }
    else  // 之前没有输出序列头，就先定位gop，再查找序列头，然后输出
    {
        // 没有gop，返回
        if(!LocateGop(user))
        {
            return;
        }
    }
    
    GetNextFrame(user);
}

bool Stream::HasVideo() const
{
    return has_video_;
}

bool Stream::HasAudio() const
{
    return has_audio_;
}

/// @brief 定位gop，查找meta,audioheader,videoheader是否在推流中接收到了
/// @param user 保存信息
/// @return 全部收到了返回true表示可以继续接受数据
bool Stream::LocateGop(const PlayerUserPtr &user)
{
    int content_lantency = user->GetAppInfo()->content_latency;
    int lantency = 0;
    auto idx = gop_mgr_.GetGopByLatency(content_lantency, lantency);
    if(idx != -1)
    {
        // 找到gop，赋值
        user->out_index_ = idx - 1;
    }
    else
    {
        // 没找到gop，判断是否超时，并且没有在超时的状态
        // TODO: http播放的时候gop超时
        auto elapse = user->ElaspsedTime();
        if(elapse >= 1000 && !user->wait_timeout_)
        {
            LIVE_DEBUG << "wait Gop keyframe timeout. host：" << user->user_id_;
            user->wait_timeout_ = true; // 防止持续输出
        }
        return false;
    }

    // 判断需不需要发meta, 是否在等待，并且有meta
    user->wait_meta_ = (user->wait_meta_ && has_meta_);
    // 推流的管理中有meta了，保存到user， 等待发送
    if(user->wait_meta_)
    {
        auto meta = codec_headers_.Meta(idx);
        if(meta)
        {
            // 有meta了，不用等待了
            user->wait_meta_ = false;
            user->meta_ = meta;
            user->meta_index_ = meta->Index();
        }
    }

    // 判断需不需要发audio header
    user->wait_audio_ = (user->wait_audio_ && has_audio_);
    // 有audio了，保存到user
    if(user->wait_audio_)
    {
        auto audioHeader = codec_headers_.AudioHeader(idx);
        if(audioHeader)
        {
            user->wait_audio_ = false;
            user->audio_header_ = audioHeader;
            user->audio_header_index_ = audioHeader->Index();
        }
    }

    // 判断需不需要发video header
    user->wait_video_ = (user->wait_video_ && has_video_);
    // 有video了，保存到user
    if(user->wait_video_)
    {
        auto videoHeader = codec_headers_.VideoHeader(idx);
        if(videoHeader)
        {
            user->wait_video_ = false;
            user->video_header_ = videoHeader;
            user->video_header_index_ = videoHeader->Index();
        }
    }

    // 有一个是true就是还需要等待，没有找到。idx也是-1表示gop也没找到
    if(user->wait_meta_ || user->wait_audio_ || user->wait_video_ || idx == -1)
    {
        // 没找到gop，判断是否超时，并且没有在超时的状态
        auto elapse = user->ElaspsedTime();
        if(elapse >= 1000 && !user->wait_timeout_)
        {
            LIVE_DEBUG << "wait Gop keyframe timeout elapsed:" << elapse
                << "ms, frame idx:" << frame_index_
                << ",gop size:" << gop_mgr_.GopSize()
                <<  "host：" << user->user_id_;
            user->wait_timeout_ = true; // 防止持续输出
        }
        return false;
    }

    // 如果上面的在这帧已经全部收到了，重置标志位等待下次的接收
    user->wait_meta_ = true;
    user->wait_audio_ = true;
    user->wait_video_ = true;
    user->out_version_ = stream_version_;

    auto elasped = user->ElaspsedTime();

    LIVE_DEBUG << "locate GOP sucess.elapsed:" << elasped
            << "ms, gop idx:" << idx
            << ",lantency:" << lantency
            << ",user:" << user->user_id_;
    
    return true;
}

/// @brief 输出时间间隔相差太大需要跳帧,gop，meta,audoiheader,videoheader都要最新的
/// @param user 
void Stream::SkipFrame(const PlayerUserPtr &user)
{
    int content_lantency = user->GetAppInfo()->content_latency;
    int lantency = 0;
    auto idx = gop_mgr_.GetGopByLatency(content_lantency, lantency);
    // 情况不一样了，如果找到了还是比当前的user的输出indx小，就跳不了
    if(idx != -1 || idx <= user->out_index_)
    {
        return;
    }

    // 每帧的meta和gop索引是一样的
    auto meta = codec_headers_.Meta(idx);
    if(meta)
    {
        // 上面gop判断完了，为了跟上新的，判断gop对应的meta是不是比较新
        if(meta->Index() > user->meta_index_)
        {
            user->meta_ = meta;
            user->meta_index_ = meta->Index();
        }
    }

    auto audio_header = codec_headers_.AudioHeader(idx);
    if(audio_header)
    {
        // 上面gop判断完了，为了跟上新的，判断gop对应的audioheader是不是比较新
        if(audio_header->Index() > user->audio_header_index_)
        {
            user->audio_header_ = audio_header;
            user->audio_header_index_ = audio_header->Index();
        }
    }

    auto video_header = codec_headers_.VideoHeader(idx);
    if(video_header)
    {
        // 上面gop判断完了，为了跟上新的，判断gop对应的meta是不是比较新
        if(video_header->Index() > user->video_header_index_)
        {
            user->video_header_ = video_header;
            user->video_header_index_ = video_header->Index();
        }
    }

    LIVE_DEBUG << "skip frame: "<< user->out_index_ << "->" << idx
            << ",lantency: " << lantency
            << ",frame_indx:" << frame_index_
            << ",host:" << user->user_id_; 
    
    // 实现跳帧
    user->out_index_ = idx - 1;
}

void Stream::GetNextFrame(const PlayerUserPtr &user)
{   
    // 下一个
    auto idx = user->out_index_ + 1;
    auto max_idx = frame_index_.load(); // 当前帧的index
    // 一次输出10个包
    for(int i = 0; i < 10; ++i)
    {
        // 不能超过当前推流上的最新帧
        if(idx > max_idx)
        {
            break;
        }

        auto &pkt = packet_buffer_[idx % packet_buffer_size_];
        if(pkt)
        {
            user->out_frames_.emplace_back(pkt);
            user->out_index_ = pkt->Index();
            user->out_frame_timestamp_ = pkt->TimeStamp();
            idx = pkt->Index() + 1;
        }
        else
        {
            break;
        }
    }
}

/// @brief 在stream中处理mpegts发送
/// @param packet 
void Stream::ProcessMpegts(PacketPtr &packet)
{
    if(CodecUtils::IsCodecHeader(packet))
    {
        char *data = packet->Data(); // Audio tags

        if(packet->IsAudio())
        {
            AudioCodecID id = (AudioCodecID)((*data & 0xf0) >> 4); // 高4位表示了音频类型MP3 aac
            encode_.SetStreamType(&write_, kVideoCodecIDReserved, id);
        }
        if(packet->IsVideo())
        {
            VideoCodecID id = (VideoCodecID)((*data & 0x0f) >> 4); // 低四位表示了音频类型MP3 aac
            encode_.SetStreamType(&write_,id, kAudioCodecIDReserved);
        }
    }

    encode_.Encode(&write_, packet, packet->TimeStamp());
}
