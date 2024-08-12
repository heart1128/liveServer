/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-11 17:08:21
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-11 18:35:12
 * @FilePath: /liveServer/src/mmedia/rtp/RtpH264.cpp
 * @Description:  learn 
 */
#include "RtpH264.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;


RtpH264::RtpH264(int32_t pt)
:Rtp(pt)
{
}

/**
 * @description: 封装H264到Rtp的body中
 * @param {list<SampleBuf>} &ins  rtmp推流上来，decode之后的H264的数据包
 * @param {uint32_t} ts
 * @param {list<PacketPtr>} &outs  编码之后的包
 * @return {*}
 */
bool RtpH264::Encode(std::list<SampleBuf> &ins, uint32_t ts, std::list<PacketPtr> &outs)
{
    timestamp_ = ts * (sample_ / 1000.0); // 原始的时间戳，转换一下
    marker_ = 0;
    int32_t header_size = HeaderSize();
    for(auto iter = ins.begin(); iter != ins.end();)
    {
        auto pre = iter;
        iter++;
        // 数据的长度大于单包了，使用组合包
        if((header_size + pre->size) > kRtpMaxPayloadSize)
        {
            EncodeFua(*pre, outs);
        }
        else
        {
            // 因为最后一个包要设置maker位，所以单独判断
            bool maker = false;
            if(iter == ins.end())
            {
                maker = true;
            }
            EncodeSingle(*pre, maker, outs);
        }
    }

    return true;
}

/**
 * @description: 封装h264的sps和pps， 使用rtp的组包模式
 * @param {string} &sps  rtmp推流进行解码得到
 * @param {string} &pps
 * @param {list<PacketPtr>} &outs
 * @return {*}
 */
bool RtpH264::EncodeSpsPps(const std::string &sps, const std::string &pps, std::list<PacketPtr> &outs)
{
    if(sps.empty() && pps.empty())
    {
        return false;
    }

    int32_t header_size = HeaderSize();
    // 5的字节是sps和pps的header长度
    int32_t payload_size = header_size + 5 + sps.size() + pps.size();
    
    PacketPtr packet = Packet::NewPacket(payload_size);
    char *header = packet->Data();
    char *payload = header + header_size;

    sequence_++;
    marker_ = 0;
    EncodeHeader(header);  // RTP头

    uint8_t type = sps[0];
    payload[0] = (type & (~0x1f)) | 24; // RTP类型是24 STAP-A
    payload += 1;

    BytesWriter::WriteUint16T(payload, sps.size());
    payload += 2;
    memcpy(payload, sps.c_str(), sps.size());

    BytesWriter::WriteUint16T(payload, pps.size());
    payload += 2;
    memcpy(payload, pps.c_str(), pps.size());

    packet->SetPacketSize(payload_size);
    packet->SetIndex(sequence_);
    packet->SetPacketType(kPacketTypeVideo);

    outs.emplace_back(packet);

    return true;
}

/**
 * @description: rtp的单包发送，
 * @param {SampleBuf} &buf  解码之后的h265单帧
 * @param {bool} last
 * @param {list<PacketPtr>} &outs
 * @return {*}
 */
bool RtpH264::EncodeSingle(const SampleBuf &buf, bool last, std::list<PacketPtr> &outs)
{
    // 头 + payload = 整个包大小
    int32_t header_size = HeaderSize();
    int32_t payload_size = header_size + buf.size;

    PacketPtr packet = Packet::NewPacket(payload_size);
    char *header = packet->Data();
    char *payload = header + header_size;
    
    sequence_++;
    marker_ = last ? 1 : 0; // 最后一个包是要设置marker的
    EncodeHeader(header);

    memcpy(payload, buf.addr, buf.size);
    packet->SetPacketSize(payload_size);
    packet->SetPacketType(kPacketTypeVideo);
    packet->SetIndex(sequence_);
    outs.emplace_back(packet);

    return true;
}

/**
 * @description: 数据包超过最大包大小，就要进行分片发送
 * @param {SampleBuf} &buf
 * @param {list<PacketPtr>} &outs
 * @return {*}
 */
bool RtpH264::EncodeFua(const SampleBuf &buf, std::list<PacketPtr> &outs)
{
    int32_t header_size = HeaderSize();
    unsigned char type = *buf.addr;     // 单帧数据，第一个字节是type
    const char *data = buf.addr + 1;
    int32_t bytes = buf.size + 1; // 还有fu indicator，fu header算在里面了
    bool start = true;

    while(bytes > 0)
    {
        // 2是分布下的两个(fu indicator和fu header)
        int32_t packet_size = header_size + 2 + bytes;
        if(packet_size > kRtpMaxPayloadSize)
        {
            packet_size = kRtpMaxPayloadSize;
        }
        PacketPtr packet = Packet::NewPacket(packet_size);
        char *header = packet->Data();
        char *fheader = header + header_size;
        char *payload = fheader + 1; // fu indicator之后
        int32_t payload_size = packet_size - 2 - header_size;

        sequence_++;
        // 数据不够了，说明是最后一个了
        if(bytes <= payload_size)
        {
            marker_ = 1;
        }
        else
        {
            marker_ = 0;
        }

        fheader[0] = (type & 0x60) | 28; // 取高2位，设置类型是28 FU-A
        fheader[1] = (type & 0x1f); // h264帧的类型，NALU的第一个字节是类型，用的是后面5位
        if(start)
        {
            start = false;
            fheader[1] |= 0x80;
        }
        else if(bytes <= payload_size)
        {
            fheader[1] |= 0x40;
        }

        memcpy(payload, data, packet_size);
        packet->SetPacketSize(packet_size);
        packet->SetPacketType(kPacketTypeVideo);
        packet->SetIndex(sequence_);
        outs.emplace_back(packet);

        data += payload_size;
        bytes -= packet_size;
    }

    return true;
}
