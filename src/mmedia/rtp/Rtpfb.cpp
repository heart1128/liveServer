/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-18 16:51:06
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:58:33
 * @FilePath: /liveServer/src/mmedia/rtp/Rtpfb.cpp
 * @Description:  learn 
 */
#include "Rtpfb.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

Rtpfb::Rtpfb()
:Rtcp(kRtcpPtPsfb)
{
}

uint32_t Rtpfb::Ssrc() const
{
    return ssrc_;
}

uint32_t Rtpfb::MediaSsrc() const
{
    return media_ssrc_;
}

/**
 * @description: 客户端丢失的包序号
 * @return {*}
 */
const std::set<uint16_t, SeqCmp> &Rtpfb::LostSeqs() const
{
    return lost_seqs_;
}

/**
 * @description: 最重要的解析客户端发送的丢失的序列号
 * @param {char} *data
 * @param {size_t} len
 * @return {*}
 */
size_t Rtpfb::DecodeNack(const char *data, size_t len)
{
    const char* p = data;
    lost_seqs_.clear();
    // PID 2位， BLP 2位
    while (len > 4)
    {
        auto pid = BytesReader::ReadUint16T(p);
        WEBRTC_DEBUG << "pid: " << pid;
        auto blp = BytesReader::ReadUint16T(p + 2);
        WEBRTC_DEBUG << "blp: " << blp;
        // 丢失的序列号，blp是掩码，如果第n位掩码为1，PID + n的数据包也要重传
        lost_seqs_.insert(pid);
        for(int i = 0; i < 16; ++i)
        {
            // 取出blp
            if((blp >> i) & 0x0001)
            {
                uint16_t npid = pid + i + 1;
                WEBRTC_DEBUG << "blp: " << blp << " npid: " << npid;
                lost_seqs_.insert(pid + i + 1);
            }
        }
        p += 4;
        len -= 4;
    }

    return p - data;
}

/**
 * @description: 解析rtpFb的数据包，里面包含NACK报文
 * @param {char} *data
 * @param {size_t} len
 * @return {*}
 */
void Rtpfb::Decode(const char *data, size_t len)
{
    // 解析rtcp的头部
    auto parsed = DecodeHeader(data, len);
    if(parsed == 0)
    {
        return;
    }
    // 跳过头部
    data += parsed;
    len -= parsed;

    // 最少有两个ssrc 4字节的
    if(len < 8)
    {
        WEBRTC_ERROR << "rtpfb no enough len: " << len;
        return;
    }

    ssrc_ = BytesReader::ReadUint32T(data);
    media_ssrc_ = BytesReader::ReadUint32T(data + 4);
    
    data += 8;
    len -= 8;

    // 在头部rc字段为1，fb的反馈包类型就是NACK
    if(rc_ == 1)
    {
        DecodeNack(data, len);
    }
}

PacketPtr Rtpfb::Encode()
{
    return PacketPtr();
}

void Rtpfb::Print()
{
}
