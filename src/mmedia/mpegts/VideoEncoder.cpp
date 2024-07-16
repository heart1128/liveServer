#include "VideoEncoder.h"
#include "mmedia/base/MMediaLog.h"
#include "TsTool.h"

using namespace tmms::mm;

namespace
{
    uint8_t *get_start_payload(uint8_t *pkt)
    {
        if(pkt[3] & 0x20) // 有适配域
        {
            // pkt[4]是适配域的长度,返回pcr起始地址
            return pkt + 5 + pkt[4];
        }
        else
        {
            return pkt + 4;
        }
    }

    // Program Clock Reference。结构为33位的低精度部分+6位的填充部分+9位的高精度部分
    int WritePcr(uint8_t *buf, int64_t pcr)
    {
        int64_t pcr_low = pcr % 300, pcr_high = pcr / 300;

        *buf++ = pcr_high >> 25;
        *buf++ = pcr_high >> 17;
        *buf++ = pcr_high >>  9;
        *buf++ = pcr_high >>  1;
        *buf++ = pcr_high <<  7 | pcr_low >> 8 | 0x7e;
        *buf++ = pcr_low;

        return 6;
    }
}

int32_t VideoEncoder::EncodeVideo(StreamWriter *writer, bool key, PacketPtr &data, int64_t dts)
{
    std::list<SampleBuf> list;
    // 从flv种解码的AVCC格式原始数据
    auto ret = demux_.OnDemux(data->Data(), data->PacketSize(), list);
    if(ret == -1)
    {
        MPEGTS_ERROR << "video demux error.";
        return -1;
    }

    writer->AppendTimeStamp(dts);
    dts = dts * 90;  // 因为MEPGTS是用90HZ的时间戳
    if(demux_.GetCodecID() == kVideoCodecIDAVC)
    {
        return EncodeAvc(writer, list, key, dts);
    }
    return 0;
}

void VideoEncoder::SetPid(uint16_t pid)
{
    pid_ = pid;
}

void VideoEncoder::SetStreamType(TsStreamType type)
{
    type_ = type;
}

int32_t VideoEncoder::EncodeAvc(StreamWriter *writer, std::list<SampleBuf> &sample_list, bool key, int64_t pts)
{
    int32_t total_size = 0;
    std::list<SampleBuf> result;
    if(demux_.HasAud()) // aud是NALU header中的分隔符
    {
        static uint8_t default_aud_nalu[] = {0x09, 0xf0}; // 固定的分隔符
        static SampleBuf default_aud_buf((const char*)&default_aud_nalu[0], 2);
        total_size += AvcInsertStartCode(result, key); // 先在前面插入stattcode, aud也要
        result.push_back(default_aud_buf);
        total_size += 2;
    }

    for(auto const &l : sample_list)
    {
        if(l.size <= 0)
        {
            MPEGTS_ERROR << "invalid avc frame length.";
            continue;
        }

        // 处理原始数据（NALU）
        auto bytes = l.addr;
        NaluType type = (NaluType)(bytes[0] & 0x1f); //NALU单元的类型
        if(type == kNaluTypeIDR && !demux_.HasSpsPps() && !sps_pps_appended_) // idr帧，没有sps或者pps，要先发sps或者Pps
        {
            auto sps = demux_.GetSPS();
            if(!sps.empty())
            {
                // 每个NALU之前都要加start code
                total_size += AvcInsertStartCode(result, key);
                result.emplace_back(sps.data(), sps.size());
                total_size += sps.size();
            }
            else
            {
                MPEGTS_ERROR << "no sps.";
            }

            auto pps = demux_.GetPPS();
            if(!sps.empty())
            {
                // 每个NALU之前都要加start code
                total_size += AvcInsertStartCode(result, key);
                result.emplace_back(pps.data(), pps.size());
                total_size += pps.size();
            }
            else
            {
                MPEGTS_ERROR << "no pps.";
            }
            sps_pps_appended_ = true; // 只发送一次
        }

        // 写入视频数据
        total_size += AvcInsertStartCode(result, key);
        result.emplace_back(l.addr, l.size);
        total_size += l.size;
    }
    // 处理dts，dts需要加上偏差时间，音频就不需要。防止视频播放卡顿
    int64_t dts = pts;
    if(demux_.GetCST() > 0)
    {
        dts = dts + demux_.GetCST() * 90;
    }
    return WriteVideoPes(writer, result, total_size, pts, dts, key);
}

int32_t VideoEncoder::AvcInsertStartCode(std::list<SampleBuf> &sample_list, bool &key)
{
   if(startcode_inserted_) // 不是第一个startcode，是插入0x000001
   {
        static uint8_t default_startcode_nalu[] = {0x00, 0x00, 0x01}; 
        static SampleBuf default_startcode_buf((const char*)&default_startcode_nalu[0], 3);
        sample_list.emplace_back(default_startcode_buf);
        return 3;
   }
   else
   {
        static uint8_t default_startcode_nalu[] = {0x00, 0x00, 0x00, 0x01}; 
        static SampleBuf default_startcode_buf((const char*)&default_startcode_nalu[0], 4);
        sample_list.emplace_back(default_startcode_buf);
        startcode_inserted_ = true;
        return 4;
   }
}

int32_t VideoEncoder::WriteVideoPes(StreamWriter *writer, std::list<SampleBuf> &result, 
                int payload_size, int64_t pts, int64_t dts, bool key)
{
    // TS固定包大小188
    uint8_t buf[188], *q;

    int32_t val = 0;
    bool is_start = true;

    while(payload_size > 0 && !result.empty())
    {
        memset(buf, 0x00, 188);

        q = buf;
        *q++ = 0x47;  // sync byte 8bits, fixed 0x47

        val = pid_ >> 8;
        if(is_start) // 是第一个包，要写palyload start 1bit
        {
            val |= 0x40;
        }
        *q++ = val;
        *q++ = pid_;
        cc_ = (cc_ + 1) & 0xf; // cc实际记录的是-1的值
        *q++ = 0x10 | cc_; // 10 适配标识

        // pes packet header，只有开始需要写头
        if(is_start)
        {
            /////// 视频帧和音频帧不一样的地方：如果是关键帧需要写pcr(在TS的适配域中)
            // 节目时钟参考（PCR，Program Clock Reference）使得解码后的内容可以正确地同步播放。
            // 因为关键帧是P,B帧的参考，所以在关键帧地方设置PCR
            if(key)
            {
                buf[3] |= 0x20; // 10打开适配域，pcr在适配域里面
                buf[4] = 1; // pcr flag
                buf[5] = 0x10;

                q = get_start_payload(buf);
                auto size = WritePcr(q, dts); // pcr就是解码时间戳，以idr帧为同步时间戳
                buf[4] += size;
                q = get_start_payload(buf);
            }

            *q++ = 0x00; // packet_start_code_prefix：24位 固定为0x00,0x00,0x01
            *q++ = 0x00;
            *q++ = 0x01;

            *q++ = 0xe0; // video stream_id ,固定值

            uint16_t header_len = 5;
            uint8_t flags = 0x02;

            if(pts != dts)  // 视频需要同时考虑dts和pts
            {
                header_len += 5;
                flags = 0x03;
            }

            int32_t len = payload_size + 5 + 3; // pes_packe_length ,在es上封装了pes头
            if(len > 0xffff)
            {
                len = 0;
            }

            *q++ = len >> 8; // ES_packet_length 16位
            *q++ = len;
            *q++ = 0x80;       
            *q++ = flags << 6;
            *q++ = header_len; 
            // PTS写入
            if(flags == 0x02) // PTS_DTS+FLAG == 10
            {
                TsTool::WritePts(q, 0x02, pts);  // 10表示 PES 头部有 PTS 字段
                q += 5;
            }
            else if(flags == 0x03) // PTS_DTS+FLAG == 11
            {
                TsTool::WritePts(q, 0x03, pts);  
                q += 5;
                TsTool::WritePts(q, 0x01, dts);  
                q += 5;
            }


            is_start = false;
        }

        int32_t header_len = q - buf; // PES + TS header
        int32_t len = 188 - header_len;
        if(len > payload_size)
        {
            len = payload_size;
        }

        int32_t stuffing = 188 - header_len - len;
        if(stuffing > 0)  // 是否需要填充
        {
            if(buf[3] & 0x20) // 是否有适配域
            {
                int32_t af_len = buf[4] + 1;
                memmove(buf + 4 + af_len +stuffing, buf + 4 + af_len , header_len - (4 + af_len));
                buf[4] += stuffing;
                memset(buf + 4 +af_len, 0xff, stuffing);
            }
            else
            {
                memmove(buf + 4 + stuffing, buf + 4 , header_len - 4);
                buf[3] |=0x20;
                buf[4] = stuffing - 1;
                if(stuffing > 2)
                {
                    buf[5] = 0x00;
                    memset(buf + 6, 0xff, stuffing - 2);
                }
            }
        }


        // 开始视频数据
        auto slen = len;
        while (slen > 0 && !result.empty())
        {
            auto &sbuf = result.front();
            if(sbuf.size <= slen) // 数据可以装下
            {
                // 数据卸载头部（已经填充了的）后面
                memcpy(buf + 188 - slen, sbuf.addr, sbuf.size);
                slen -= sbuf.size;
                result.pop_front();
            }
            else
            {
                // 写一段进去
                memcpy(buf + 188 - slen, sbuf.addr, slen);
                sbuf.addr += slen;
                sbuf.size -= slen;
                slen = 0;
                break;
            }
        }
        payload_size -= len;
        writer->Write(buf, 188);

    }
    return 0;
}
