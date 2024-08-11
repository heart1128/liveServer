/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 18:55:50
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-09 19:05:40
 * @FilePath: /liveServer/src/mmedia/rtp/RtpOpus.cpp
 * @Description:  learn 
 */
#include "RtpOpus.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

RtpOpus::RtpOpus(int32_t pt)
:Rtp(pt)
{
    sample_ = 48000;
}

/**
 * @description: rtp的opus一帧数据是直接放在header后面的
 * @param {list<SampleBuf>} &ins 编码成opus之后的数据
 * @param {uint32_t} ts
 * @param {list<PacketPtr>} &outs
 * @return {*}
 */
bool RtpOpus::Encode(std::list<SampleBuf> &ins, uint32_t ts, std::list<PacketPtr> &outs)
{
    int32_t header_size = HeaderSize();
    // 每帧都发一个包，每帧都有头
    for(auto const &s : ins)
    {
        int32_t payload_size = header_size + s.size;

        PacketPtr packet = Packet::NewPacket(payload_size);
        char *header = packet->Data();
        char *payload = header + header_size;

        timestamp_ += 10 * (sample_ / 1000.0); // 10ms音频采样对应的时间量
        sequence_ = sequence_ + 1;
        marker_ = 1;  // 音频表示会话的开始
        EncodeHeader(header);

        memcpy(payload, s.addr, s.size);
        packet->SetPacketSize(payload_size);
        packet->SetPacketType(kPacketTypeAudio);
        packet->SetIndex(sequence_);
        outs.emplace_back(std::move(packet));
    }
    return true;
}
