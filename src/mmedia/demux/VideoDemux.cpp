/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-09 14:22:36
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-09 15:17:08
 * @FilePath: /liveServer/src/mmedia/demux/VideoDemux.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#include "VideoDemux.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

int32_t VideoDemux::OnDemux(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    VideoCodecID id = (VideoCodecID)(*data & 0x0f);
    // 只做了AVC格式支持（H.264）
    if(id != kVideoCodecIDAVC)
    {
        DEMUX_ERROR << "not support video type:" << id;
        return -1;
    }
    return DemuxAVC(data, size, outs);
}

bool VideoDemux::HasIdr() const
{
    return has_idr_;
}

bool VideoDemux::HasAud() const
{
    return has_aud_;
}

bool VideoDemux::HasSpsPps() const
{
    return has_pps_sps_;
}

int32_t VideoDemux::DemuxAVC(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    // tag body开始的1字节
    uint8_t ftype = (*data & 0xf0) >> 4;  // 帧类型
    if(ftype == kFrameTypeVideoInfoFrame)   // 信息不处理
    {
        DEMUX_DEBUG << "igore info frame.";
        return 0;
    }

    // 当codecid == 7时，tag body，第一个字节为AvcPacketType，第二三四个字节为CompositionTime
    // 当AvcPacketType=0，第5个字节开始为AVCDecoderConfigurationRecord（开始解析序列头）；否则VideoData为Avc Raw数据(NALU)
    uint8_t avc_packet_type = data[1];
    int32_t cst = BytesReader::ReadUint24T(data + 2);
    // DEMUX_DEBUG << "cst: " << cst;
    composition_time_ = cst;
    if(avc_packet_type == 0)
    {
        return DecodeAVCSeqHeader(data + 5, size - 5, outs);
    }
    else if(avc_packet_type == 1)
    {
        return DecodeAvcNalu(data + 5, size - 5, outs);
    }
    else
    {
        return 0;
    }
}

const char *VideoDemux::FindAnnexbNalu(const char *p, const char *end)
{
    for(p += 2; p + 1 < end; p++)
    {
        // 3字节 0x 00 00 01
        if(*p == 0x01 && *(p -1) == 0x00 && *(p - 2) == 0x00)
        {
            return p + 1;
        }
    }
    return end;
}

// 适用流式
int32_t VideoDemux::DecodeAVCNaluAnnexb(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    // start code(3字节或者4字节) + NALU
    // 起始码在NALU为SPS、PPS或NALU为一帧的第一个NALU时使用4字节，其他情况使用3字节
    if(size < 3)
    {
        DEMUX_ERROR << "error annexb bytes:" << size;
        return -1;
    }
    int32_t ret = -1;
    const char* data_end = data + size;
    // 去掉start code
    const char* nalu_start = FindAnnexbNalu(data, data_end);
    while(nalu_start < data_end)
    {
        // 下一个起始就是上一个结束
        const char * nalu_next = FindAnnexbNalu(nalu_start + 1, data_end);
        // annexb没有长度字段，要自己计算
        int32_t nalu_size = nalu_next - nalu_start;

        if(nalu_size > size || size <= 0)    //  长度出错
        {
            DEMUX_ERROR << "error annexb nalu bytes:" << size << " nalu size:" << nalu_size;
            return -1;
        }

        ret = 0;
        outs.emplace_back(SampleBuf(data, nalu_size));
        CheckNaluType(nalu_start);
        data += nalu_size;
        size -= nalu_size;

        nalu_start = nalu_next;
    }
    return ret;
}

int32_t VideoDemux::DecodeAVCNaluIAvcc(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    // 长度 + NALU ... 长度 + NALU
    while(size > 1)
    {
        uint32_t nalu_size = 0;
        if(nalu_unit_length_ == 3) // 记录值是-1的，所以3表示4字节
        {
            nalu_size = BytesReader::ReadUint32T(data);
        }
        else if(nalu_unit_length_ == 1)
        {
            nalu_size = BytesReader::ReadUint16T(data);
        }
        else
        {
            nalu_size = data[0];
        }

        data += nalu_unit_length_ + 1;
        size -= nalu_unit_length_ + 1;

        if(nalu_size > size || size <= 0)    //  长度出错
        {
            DEMUX_ERROR << "error avcc nalu bytes:" << size << " nalu size:" << nalu_size;
            return -1;
        }

        // 整理成一个个NALU包
        outs.emplace_back(SampleBuf(data, nalu_size));
        CheckNaluType(data);
        data += nalu_size;
        size -= nalu_size;
    }

    return 0;
}

int32_t VideoDemux::DecodeAVCSeqHeader(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    if(size < 5) // 序列头至少5字节
    {
        DEMUX_ERROR << "seq header size error.size:" << size;
        return -1;
    }

    config_version_ = data[0];
    profile_ = data[1];
    profile_com_ = data[2];
    level_ = data[3];
    
    nalu_unit_length_ = data[4] & 0x03; // 2位, 定义了NALU长度的字节数，真实值是-1的
    DEMUX_DEBUG << "nalu_unit_length:" << nalu_unit_length_;

    data += 5;
    size -= 5;

    // 一个字节的sps个数，一个sps的长度，一个sps内容，至少的
    if(size < 3)
    {
        DEMUX_ERROR << "seq header size error.no found sps:" << size;
        return -1;
    }

    int8_t sps_num = data[0] & 0x1F; // 5位
    if(sps_num != 1)        // 只做了处理一个sps，多了定义为错误
    {
        DEMUX_ERROR << "more than 1 sps.";
        return -1;
    }
    int16_t sps_length = BytesReader::ReadUint16T(data + 1); // 16bit
    if(sps_length > 0 && sps_length < size - 3)
    {
        DEMUX_DEBUG << "found sps, bytes:" << sps_length;
    }
    else
    {
        DEMUX_ERROR << "sps length error.sps_length:" << sps_length << " size:" << size;
        return -1;
    }
    sps_.assign(data + 3, sps_length);

    data += 3;
    size -= 3;
    data += sps_length;
    size -= sps_length;
    
    // 一个字节的pps个数，一个pps的长度，一个pps内容，至少的
    if(size < 3)
    {
        DEMUX_ERROR << "seq header size error.no found pps:" << size;
        return -1;
    }

    int8_t pps_num = data[0] & 0x1F; // 5位
    if(pps_num != 1)        // 只做了处理一个pps，多了定义为错误
    {
        DEMUX_ERROR << "more than 1 pps.";
        return -1;
    }
    int16_t pps_length = BytesReader::ReadUint16T(data + 1); // 16bit
    if(pps_length > 0 && pps_length < size - 3)
    {
        DEMUX_DEBUG << "found pps, bytes:" << pps_length;
    }
    else
    {
        DEMUX_ERROR << "pps length error.pps_length:" << pps_length << " size:" << size;
        return -1;
    }
    pps_.assign(data + 3, pps_length);
    return 0;
}

int32_t VideoDemux::DecodeAvcNalu(const char *data, size_t size, std::list<SampleBuf> &outs)
{
    // AVC两种格式，不知道的话就要判断，知道就直接指定
    if(payload_format_ == kPayloadFormatUnkonwed)
    {
        if(!DecodeAVCNaluIAvcc(data, size ,outs))
        {
            payload_format_ = kPayloadFormatAvcc;
        }
        else
        {
            if(!DecodeAVCSeqHeader(data, size, outs))
            {
                payload_format_ = kPayloadFormatAnnexB;
            }
            else
            {
                DEMUX_ERROR << "payload format error.no found format.";
                return -1;
            }
        }
    }
    else if(payload_format_ == kPayloadFormatAvcc)
    {
        return DecodeAVCNaluIAvcc(data, size, outs);
    }
    else if(payload_format_ == kPayloadFormatAnnexB)
    {
        return DecodeAVCNaluAnnexb(data, size, outs);
    }
    return 0;
}

void VideoDemux::CheckNaluType(const char *data)
{
    NaluType type = (NaluType)(data[0] & 0x1f);
    if(type == kNaluTypeIDR)
    {
        has_idr_ = true;
    }
    else if(type == kNaluTypeAccessUnitDelimiter)
    {
        has_aud_ = true;
    }
    else if(type == kNaluTypeSPS || type == kNaluTypePPS)
    {
        has_pps_sps_ = true;
    }
}
