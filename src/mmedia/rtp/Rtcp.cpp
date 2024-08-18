/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-18 15:47:12
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-18 16:10:52
 * @FilePath: /liveServer/src/mmedia/rtp/Rtcp.cpp
 * @Description:  learn 
 */
#include "Rtcp.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
using namespace tmms::mm;

Rtcp::Rtcp(int32_t pt)
:version_(2),
padding_(0),
rc_(0),
payload_type_(pt),
pl_len_(0)
{
}

/**
 * @description: 解释rtcp的头部
 * @param {char} *data
 * @param {size_t} len
 * @return {*}
 */
int32_t Rtcp::DecodeHeader(const char *data, size_t len)
{
    // 最少四字节
    if(len < 4)
    {
        return 0;
    }
    version_ = data[0] >> 6;
    padding_ = (data[0] >> 5) & 0x01;
    rc_ = (data[0] & 0x1f);
    payload_type_ = data[1];
    data += 2;
    pl_len_ = BytesReader::ReadUint16T(data);
    return 4;
}

int32_t Rtcp::EncodeHeader(char *buf)
{
    buf[0] = (char)((version_ << 6) | (padding_ << 5) | rc_);
    buf[1] = (char)payload_type_;

    BytesWriter::WriteUint16T(buf + 2, pl_len_);
    return 4;
}

void Rtcp::SetRC(int count)
{
}

/**
 * @description: 的PayloadType为200，用于发送端向接收端提供关于发送端自身和本地时间戳的信息。
 * @return {*}
 */
int Rtcp::RC() const
{
    return 0;
}

void Rtcp::SetPayloadType(int type)
{
}

int Rtcp::PayloadType() const
{
    return 0;
}

void Rtcp::SetPadding(bool pad)
{
}

bool Rtcp::Padding() const
{
    return false;
}

void Rtcp::SetPayloadLength(uint32_t size)
{
}

uint32_t Rtcp::PayloadLength() const
{
    return 0;
}
