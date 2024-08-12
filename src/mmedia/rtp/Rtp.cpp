/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 17:13:24
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 17:00:10
 * @FilePath: /liveServer/src/mmedia/rtp/Rtp.cpp
 * @Description:  learn 
 */
#include "Rtp.h"
#include "mmedia/base/MMediaLog.h"
#include <netinet/in.h>
#include <cstring>

using namespace tmms::mm;

Rtp::Rtp(int32_t pt)
:version_(2), padding_(0), extern_(0), csrc_count_(0), marker_(0),
payload_type_(pt), sequence_(0), timestamp_(0), ssrc_(0), sample_(0)
{
}

/**
 * @description: 对于视频，标记一帧的结束；对于音频，标记会话的开始
 * @param {bool} on
 * @return {*}
 */
void Rtp::SetMarker(bool on)
{
    marker_ = on ? 1 : 0;
}

void Rtp::SetSequenceNumber(uint16_t s)
{
    sequence_ = s;
}

uint16_t Rtp::SequenceNumber() const
{
    return sequence_;
}

void Rtp::SetTimestamp(uint32_t timestamp)
{
    timestamp_ = timestamp;
}

uint32_t Rtp::Timestamp() const
{
    return timestamp_;
}

/**
 * @description: ssrc是
 * @param {uint32_t} ssrc
 * @return {*}
 */
void Rtp::SetSsrc(uint32_t ssrc)
{
    ssrc_ = ssrc;
}

uint32_t Rtp::Ssrc() const
{
    return ssrc_;
}

void Rtp::SetSample(int32_t s)
{
    sample_ = s;
}

/**
 * @description: 组装头
 * @param {char} *buf
 * @return {*}
 */
void Rtp::EncodeHeader(char *buf)
{
    char *header = buf;

    // 第一个字节
    // 单字节没有大小端
    header[0] = (char)(version_ << 6 | padding_ << 5 | extern_ << 4 | csrc_count_);
    header[1] = (char)(marker_ << 7 | payload_type_);
    header[2] = (char)(sequence_ >> 8);
    header[3] = (char)(sequence_ & 0xff);
    header += 4;

    // 转网络大端
    uint32_t t = htonl(timestamp_);
    std::memcpy(header, &t, sizeof(uint32_t));
    header += 4;

    uint32_t ss = htonl(ssrc_);
    std::memcpy(header, &ss, sizeof(uint32_t));
    header += 4;

    // 处理贡献源
    // for(int i = 0; i < csrc_count_; ++i)
    // {
    //     uint32_t ss = htonl(ssrc_);
    //     std::memcpy(header, &ss, sizeof(uint32_t));
    //     header += 4;
    // }
}

int32_t Rtp::HeaderSize() const
{
    int32_t header_size = 12;
    // 12 是没有ssrc和scrc的，
    // 每个CSRC标识符占32位，可以有0~15个CSRC。每个CSRC标识了包含在RTP报文有效载荷中的所有提供信源。
    if(csrc_count_ > 0)
    {
        header_size += csrc_count_ * 4;
    }
    return header_size;
}
