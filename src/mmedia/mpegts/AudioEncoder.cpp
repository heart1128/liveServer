#include "AudioEncoder.h"
#include "mmedia/base/MMediaLog.h"
#include "TsTool.h"
using namespace tmms::mm;

namespace 
{
    // flv的adif格式的objectType对应的ADTS格式的profileType
    AacProfile AacObjectType2AacProfile(AACObjectType type)
    {
        switch (type)
        {
        case kAACObjectTypeAacMain:
            return AacProfileMain;
        case kAACObjectTypeAacHE:
        case kAACObjectTypeAacHEV2:
        case kAACObjectTypeAacLC:
            return AacProfileLC;
        case kAACObjectTypeAacSSR:
            return AacProfileSSR;
        default:
            break;
        }
        return AacProfileReserved; // 默认返回非法
    }
} // namespace 


int32_t AudioEncoder::EncodeAudio(StreamWriter *writer, PacketPtr &data, int64_t dts)
{
    std::list<SampleBuf> list;
    // 解析flv的tag body的AUDIODATA
    // 返回的list是音频的raw数据
    auto ret = demux_.OnDemux(data->Data(), data->PacketSize(), list);
    if(ret == -1)
    {
        MPEGTS_ERROR << "audio demux err.";
        return -1;
    }

    // 头信息就不要了
    if(TsTool::IsCodecHeader(data))
    {
        return 0;
    }

    writer->AppendTimeStamp(dts);  // 更新解码时间戳
    dts = dts * 90;                 // ffmpeg用的是90HZ的系统时间
    if(demux_.GetCodecId() == kAudioCodecIDAAC)
    {
        return EncodeAAC(writer, list, dts);
    }
    else if(demux_.GetCodecId() == kAudioCodecIDMP3)
    {
        return EncodeMP3(writer, list, dts);
    }
    return 0;
}


int32_t AudioEncoder::EncodeAAC(StreamWriter *writer, std::list<SampleBuf> &sample_list, int64_t pts)
{
    for(auto const &l : sample_list)
    {
        // size最多13位？  // TODO为什么是最大13位的，在tag header看好像最多24位
        if(l.size < 0 || l.size > 0x1fff)
        {
            MPEGTS_ERROR << "invalid aac frame length.";
            return -1;
        }

        int32_t frame_length = 7 + l.size; // 头部 + 数据 = frame
        // 拿到了flv解析的原始数据，组装ADTS头
        std::list<SampleBuf> result;
        // fixed + variable
        uint8_t adts_header[7] = {0xff, 0xf9, 0x00, 0x00, 0x0f, 0xfc}; // 初始化随机值

        AacProfile profile = AacObjectType2AacProfile(demux_.GetObjectType());

        adts_header[2] = (profile << 6) & 0xc0; // 高2位
        adts_header[2] |= (demux_.GetSampleRateIndex() << 2) & 0x3c; // 4位
        adts_header[2] |= (demux_.GetChannel() >> 2) & 0x01; // 1位
        adts_header[3] = (demux_.GetChannel() << 6) & 0xc0;  // channel

        adts_header[3] |= (frame_length >> 11) & 0x03;
        adts_header[4] = (frame_length >> 3) & 0xff;
        adts_header[5] = (frame_length << 5) & 0xe0; // 3位 移5位取3位

        adts_header[5] |= 0x1f;

        // header + body
        result.emplace_back(SampleBuf((const char*)adts_header, 7));
        result.emplace_back(SampleBuf(l.addr, l.size));

        // 写入PES,对ES的封装，可以在TS中传输
        return WriteAudioPes(writer, result, 7 + l.size, pts);
    }
    return 0;
}

int32_t AudioEncoder::EncodeMP3(StreamWriter *write, std::list<SampleBuf> &sample_list, int64_t pts)
{
    int32_t size = 0;
    // 不用转，计算长度就行
    for(auto const &l : sample_list)
    {
        size += l.size;
    }

    return WriteAudioPes(write, sample_list, size, pts);
}

int32_t AudioEncoder::WriteAudioPes(StreamWriter *write, std::list<SampleBuf> &result, int payload_size, int64_t dts)
{
    // TS固定包大小188
    uint8_t buf[188], *q;

    int32_t val = 0;
    bool is_start = true;

    while(payload_size > 0 && !result.empty())
    {
        memset(buf, 0xff, 188);

        q = buf;
        *q++ = 0x47;  // sync byte 8bits, fixed 0x47

        val = pid_ >> 8;
        if(is_start) // 是第一个包，要写palyload start 1bit
        {
            val |= 0x40;
        }
        *q++ = val;
        *q++ = pid_;
        cc_ = (cc_ + 1) & 0xff; // cc实际记录的是-1的值
        *q++ = 0x10 | cc_; // 10 适配标识

        // pes packet header，只有开始需要写头
        if(is_start)
        {
            *q++ = 0x00; // packet_start_code_prefix：24位 固定为0x00,0x00,0x01
            *q++ = 0x00;
            *q++ = 0x01;

            *q = 0xc0; // stream_id ,固定值

            int32_t len = payload_size + 5 + 3; // pes_packe_length ,在es上封装了pes头
            if(len > 0xffff)
            {
                len = 0;
            }

            *q++ = len >> 8; // ES_packet_length 16位
            *q++ = len;
            *q++ = 0x80;       
            *q++ = 0x02 << 6;
            *q++ = 5; 
            // PTS写入
            TsTool::WritePts(q, 0x02, dts);  // 10表示 PES 头部有 PTS 字段
            q += 5;

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


        // 开始音频数据
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
        write->Write(buf, 188);

    }
    return 0;
}

void AudioEncoder::SetPid(uint16_t pid)
{
    pid_ = pid;
}

void AudioEncoder::SetStreamType(TsStreamType type)
{
    type_ = type;
}
