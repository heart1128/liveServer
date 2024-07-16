/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-15 15:58:55
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-15 17:26:09
 * @FilePath: /liveServer/src/mmedia/mpegts/TsEncoder.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#include "TsEncoder.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

int32_t TsEncoder::Encode(StreamWriter *writer, PacketPtr &data, int64_t dts)
{
    if(data->IsAudio())
    {
        return audio_encoder_.EncodeAudio(writer, data, dts);
    }
    else if(data->IsVideo())
    {
        bool key = data->IsKeyFrame();
        return video_encoder_.EncodeVideo(writer, key, data, dts);
    }
    return 0;
}

void TsEncoder::SetStreamType(StreamWriter *w, VideoCodecID vc, AudioCodecID ac)
{
    TsStreamType atype = kTsStreamReserved;
    TsStreamType vtype = kTsStreamReserved;

    if(ac == kAudioCodecIDAAC)
    {
        atype = kTsStreamAudioAAC;
        audio_pid_ = 0x101;  // 音频固定的
    }
    if(ac == kAudioCodecIDMP3)
    {
        atype = kTsStreamAudioMp3;
        audio_pid_ = 0x102;  // 音频固定的
    }

    if(vc == kVideoCodecIDAVC)
    {
        vtype = kTsStreamVideoH264;
        video_pid_ = 0x100;  // 固定的
    }

// 要不要更新Pmt
    bool writer = false;
    if(atype != kTsStreamReserved && atype != audio_type_)
    {
        auto p = std::make_shared<ProgramInfo>();  // 创建一个节目
        p->elementary_pid = audio_pid_;
        p->stream_type = atype;
        pmt_writer_.AddProgramInfo(p);
        writer = true;
        audio_encoder_.SetPid(audio_pid_);
        audio_encoder_.SetStreamType(atype);
        audio_type_ = atype;
    }
    if(vtype != kTsStreamReserved && vtype != video_type_)
    {
        auto p = std::make_shared<ProgramInfo>();  // 创建一个节目
        p->elementary_pid = vtype;
        pmt_writer_.AddProgramInfo(p);
        writer = true;
        video_encoder_.SetPid(video_pid_);
        video_encoder_.SetStreamType(vtype);
        pmt_writer_.SetPcrPid(video_pid_); // 设置pcr
        video_type_ = vtype;
    }

    if(writer)
    {
        WritePatPmt(w);
    }
}

int32_t TsEncoder::WritePatPmt(StreamWriter *w)
{
    if(audio_pid_ != 0xe000 && video_pid_ != 0xe000)
    {
        pat_writer_.WritePat(w);
        pmt_writer_.Writepmt(w);
    }

    return 0;
}
