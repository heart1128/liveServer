/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 18:34:26
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 17:38:23
 * @FilePath: /liveServer/src/mmedia/rtp/TOpusEncoder.cpp
 * @Description:  learn 
 */
#include "TOpusEncoder.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

TOpusEncoder::~TOpusEncoder()
{
    if(encoder_)
    {
        opus_encoder_destroy(encoder_);
        encoder_ = nullptr;
    }
}

bool TOpusEncoder::Init(int sample, int channels)
{
    int error = 0;
    // 采样率，通道
    encoder_ = opus_encoder_create(sample, channels, OPUS_APPLICATION_AUDIO, &error);
    if(!encoder_)
    {
        WEBRTC_ERROR << "opus encoder create failed.";
        return false;
    }
    // 接受丢包20%
    opus_encoder_ctl(encoder_, OPUS_SET_PACKET_LOSS_PERC(20));
    // 内置前向纠错（FEC）功能。FEC是一种错误纠正技术，可以在数据传输过程中检测并纠正错误，
    opus_encoder_ctl(encoder_, OPUS_SET_INBAND_FEC(1));

    //帧占用字节opus_uint16 16bit采样
    frame_bytes_ = frame_size_ * channels_ * sizeof(opus_uint16);
    return true;
}

/**
 * @description: 对pcm进行编码成opus
 * @param {SampleBuf} &pcm
 * @param {list<PacketPtr>} &pkts
 * @return {*}
 */
bool TOpusEncoder::Encode(SampleBuf &pcm, std::list<PacketPtr> &pkts)
{
    memcpy(pcm_buf_ + pcm_bytes_, pcm.addr, pcm.size);
    pcm_bytes_ += pcm.size;

    // 数据不够
    if(pcm_bytes_ < frame_bytes_)
    {
        return false;
    }

    unsigned char out_buffer[kOpusMaxSize];
    
    // 不断编码
    while(pcm_bytes_ >= frame_bytes_)
    {
        // 编码一帧
        auto bytes = opus_encode(encoder_, (const opus_int16*)pcm_buf_, frame_size_, out_buffer, kOpusMaxSize);
        pcm_bytes_ -= frame_bytes_;
        // 剩下的往前移动
        memmove(pcm_buf_, pcm_buf_ + frame_bytes_, pcm_bytes_);

        if(bytes > 0)
        {
            // 编码的一帧数据加入到list
            PacketPtr packet = Packet::NewPacket(bytes);
            memcpy(packet->Data(), out_buffer, bytes);
            packet->SetPacketSize(bytes);
            packet->SetPacketType(kPacketTypeAudio);
            pkts.emplace_back(packet);
        }
    }
    return !pkts.empty();
}
