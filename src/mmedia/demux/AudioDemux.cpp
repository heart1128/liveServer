/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-09 11:19:37
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-09 13:42:53
 * @FilePath: /liveServer/src/mmedia/demux/AudioDemux.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "AudioDemux.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/NalBitStream.h"

using namespace tmms::mm;

/// @brief 解析flv的tag body的AUDIODATA
/// @param data 
/// @param size 
/// @param list 
/// @return  正确返回0，错误返回-1
int32_t AudioDemux::OnDemux(const char *data, size_t size, std::list<SampleBuf> &list)
{
    if(size < 1)
    {
        DEMUX_ERROR << "param error.size < 1";
        return -1;
    }
    // 高四位
    sound_format_ = (AudioCodecID)((*data >> 4)&0x0f);
    sound_rate_ = (SoundRate)((*data & 0xc0) >> 2);
    sound_size_ = (SoundSize)((*data & 0x02) << 1);
    sound_type_ = (SoundChannel)(*data & 0x01);
    DEMUX_DEBUG << "format:" << sound_format_
                << " rate:" << sound_rate_
                << " size:" << sound_size_
                << " type:" << sound_type_;
    
    // mp3是2
    if(sound_format_ == kAudioCodecIDMP3)
    {
        return DemuxMP3(data, size, list);
    }
    else if(sound_format_ == kAudioCodecIDAAC)  // 10
    {
        return DemuxAAC(data, size, list);
    }
    else
    {
        DEMUX_ERROR << "not surpport code id: " << sound_format_;
    }
    return 0;
}

/// @brief AAC第二个字节开始是AACPacketType，额外描述AAC格式
/// @param data 
/// @param size 
/// @param list 
/// @return 
int32_t AudioDemux::DemuxAAC(const char *data, size_t size, std::list<SampleBuf> &list)
{
    AACPacketType type = (AACPacketType)data[1];

    if(type == kAACPacketTypeAACSequenceHeader)
    {
        // 1字节是tag header  1字节是avPacketType ,剩下的就是序列头数据
        if(size - 2 > 0)
        {
            return DemuxAACSequenceHeadr(data + 2, size - 2);
        }
    }
    else if(type == kAACPacketTypeAACRaw)
    {
        if(!aac_ok_)
        {
            return -1;
        }
        list.emplace_back(data + 2, size - 2);
    }
    return 0;
}

/// @brief mp3的tag body除了第一个信息字节，剩下的就是原始数据
/// @param data 
/// @param size 
/// @param list 
/// @return 
int32_t AudioDemux::DemuxMP3(const char *data, size_t size, std::list<SampleBuf> &list)
{
    list.emplace_back(SampleBuf(data + 1, size - 1));
    return 0;
}

/// @brief aac比mp3多个额外的序列头，存放采样率，声道数等数据
/// @param data 
/// @param size 
/// @return 
int32_t AudioDemux::DemuxAACSequenceHeadr(const char *data, int size)
{
    if(size < 2)  // 头部都至少两个字节
    {
        DEMUX_ERROR << "demux aac seq header failed. size < 2";
        return -1;
    }

    NalBitStream stream(data, size);

    aac_object_ = (AACObjectType)stream.GetWord(4);
    aac_sample_rate_ = stream.GetWord(5);
    aac_channel = stream.GetWord(4);

    aac_ok_ = true;
    return 0;
}
