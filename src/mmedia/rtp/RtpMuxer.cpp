/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-12 16:52:59
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 17:45:14
 * @FilePath: /liveServer/src/mmedia/rtp/RtpMuxer.cpp
 * @Description:  learn 
 */
#include "RtpMuxer.h"
#include "RtpOpus.h"
#include "RtpH264.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/mpegts/TsTool.h"

using namespace tmms::mm;
bool RtpMuxer::Init(int32_t vpt, int32_t apt, uint32_t vssrc, uint32_t assrc)
{
    audio_rtp_ = std::make_shared<RtpOpus>(apt);
    video_rtp_ = std::make_shared<RtpH264>(vpt);
    audio_demux_ = std::make_shared<AudioDemux>();
    video_demux_ = std::make_shared<VideoDemux>();
    audio_rtp_->SetSsrc(assrc);
    video_rtp_->SetSsrc(vssrc);
    return true;
}

/**
 * @description: 对传入的rtmp数据包进行解析，然后转成rtp数据包
 * @param {PacketPtr} &pkt  原始的rtmp推流的h264数据包
 * @param {list<PacketPtr>} &rtp_pkts  不传入数据，带出数据
 * @param {uint32_t} timestamp
 * @return {*}
 */
int32_t RtpMuxer::EncodeVideo( PacketPtr &pkt, std::list<PacketPtr> &rtp_pkts, uint32_t timestamp)
{
    std::list<SampleBuf> list;
    video_demux_->Reset();
    // 使用demux解析原始的h264，每帧放在一个sampleBuf中
    auto ret = video_demux_->OnDemux(pkt->Data(), pkt->PacketSize(), list);
    if(ret == -1)
    {
        WEBRTC_ERROR << "video demux error.";
        return -1;
    }
    // 是头，不编码
    if(TsTool::IsCodecHeader(pkt))
    {
        return 0;
    }
    // 是b帧，webrtc不需要b帧
    if(video_demux_->HasBframe())
    {
        return 0;
    }
    // 没有发送过sps和pps，并且是idr帧，并且当前帧不是sps和pps
    if(!sent_sps_pps_ && video_demux_->HasIdr() && !video_demux_->HasSpsPps())
    {
        sent_sps_pps_ = true;
        // video_rtp是Rtp类型，是RtpH264的基类
        auto h264_rtp = std::dynamic_pointer_cast<RtpH264>(video_rtp_);
        h264_rtp->EncodeSpsPps(video_demux_->GetSPS(), video_demux_->GetPPS(), rtp_pkts);
    }
    if(video_demux_->HasSpsPps())
    {
        sent_sps_pps_ = true;
    }  
    // 对h264数据进行编码成rtp数据包
    auto r = video_rtp_->Encode(list, timestamp, rtp_pkts);
    return r ? 0 : -1;
}

/**
 * @description: 对AAC数据进行解码成pcm，pcm编码成opus，编码到rtp的body中
 * @param {PacketPtr} &pkt
 * @param {list<PacketPtr>} &rtp_pkts
 * @param {uint32_t} timestamp
 * @return {*}
 */
int32_t RtpMuxer::EncodeAudio(PacketPtr &pkt, std::list<PacketPtr> &rtp_pkts, uint32_t timestamp)
{
    std::list<SampleBuf> list;
    // 使用demux解析原始的AAC，每帧放在一个sampleBuf中
    auto ret = audio_demux_->OnDemux(pkt->Data(), pkt->PacketSize(), list);
    if(ret == -1)
    {
        WEBRTC_ERROR << "audio demux error.";
        return -1;
    }
    // 没有收到aac序列头的，
    if(audio_demux_->AACSeqHeader().empty())
    {
        return 0;
    }
    // 先解码aac
    if(!aac_decoder_)
    {
        aac_decoder_ = std::make_shared<AACDecoder>();
        aac_decoder_->Init(audio_demux_->AACSeqHeader());
    }
    // 开始编码成opus
    if(!opus_encoder_)
    {
        opus_encoder_ = std::make_shared<TOpusEncoder>();
        opus_encoder_->Init(audio_demux_->GetSampleRate(), audio_demux_->GetChannel());
    }
    for(auto &l : list)
    {
        // 先把aac转成pcm，再转成opus
        auto pcm = aac_decoder_->Decode((unsigned char*)l.addr, l.size);
        if(pcm.size > 0)
        {
            std::list<PacketPtr> outs;
            auto r = opus_encoder_->Encode(pcm, outs);
            if(r)
            {
                // 把packetPtr转成SampleBuf
                std::list<SampleBuf> result;
                for(auto &const o : outs)
                {
                    result.emplace_back(o->Data(), o->PacketSize());
                }
                // 封装到body中
                audio_rtp_->Encode(result, timestamp, rtp_pkts);
            }
        }
    }
    return 1;
}

uint32_t RtpMuxer::VideoTimestamp() const
{
    return 0;
}

uint32_t RtpMuxer::AudioTimestamp() const
{
    return 0;
}

uint32_t RtpMuxer::VideoSsrc() const
{
    return 0;
}

uint32_t RtpMuxer::AudioSsrc() const
{
    return 0;
}
