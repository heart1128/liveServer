#include "CodecHeader.h"
#include "base/TTime.h"
#include "live/base/LiveLog.h"
#include "mmedia/rtmp/amf/AMFObject.h"
#include <sstream>

using namespace tmms::live;
using namespace tmms::base;


CodecHeader::CodecHeader()
{
    start_timestamp_ = TTime::NowMS();
}

CodecHeader::~CodecHeader()
{
}

/// @brief 查找meta
/// @param idx 
/// @return 
PacketPtr CodecHeader::Meta(int idx)
{
    // 如果是第一个，直接返回
    if(idx <= 0)
    {
        return meta_;
    }

    // 播放是后往前，一般都是查找最新的，后面开始快点
    auto iter = meta_packets_.rbegin();
    for(; iter != meta_packets_.rend(); iter++)
    {
        // 查找第一个比他小的
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    return meta_;
}

PacketPtr CodecHeader::VideoHeader(int idx)
{
     // 如果是第一个，直接返回
    if(idx <= 0)
    {
        return video_header_;
    }

    // 播放是后往前，一般都是查找最新的，后面开始快点
    auto iter = video_header_packets_.rbegin();
    for(; iter != video_header_packets_.rend(); iter++)
    {
        // 查找第一个比他小的
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    return video_header_;
}

PacketPtr CodecHeader::AudioHeader(int idx)
{
   // 如果是第一个，直接返回
    if(idx <= 0)
    {
        return audio_header_;
    }

    // 播放是后往前，一般都是查找最新的，后面开始快点
    auto iter = audio_header_packets_.rbegin();
    for(; iter != audio_header_packets_.rend(); iter++)
    {
        // 查找第一个比他小的
        PacketPtr pkt = *iter;
        if(pkt->Index() <= idx)
        {
            return pkt;
        }
    }

    return audio_header_;
}

/// @brief 
/// @param packet 
void CodecHeader::SaveMeta(const PacketPtr & packet)
{
    meta_ = packet;
    ++meta_version_; // 新的meta

    meta_packets_.emplace_back(packet);

    LIVE_TRACE << "save meta, meta version:" << meta_version_
                << ", size:" << packet->PacketSize()
                << " elapse:" << TTime::NowMS() - start_timestamp_ << "ms\n";
}

/// @brief meta也是用AMF编码的（属于rtmp传输的数据），所以解析amf
/// @param packet 
void CodecHeader::ParseMeta(const PacketPtr &packet)
{
    AMFObject obj;
    if(!obj.Decode(packet->Data(), packet->PacketSize()))
    {
        return;
    }

    std::stringstream ss;
    ss << "ParseMeta ";
    
    // obj继承any
    AMFAnyPtr widthPtr = obj.Property("width");
    if(widthPtr)
    {
        ss << ", width:" << (uint32_t)widthPtr->Number();
    }

    AMFAnyPtr heightPtr = obj.Property("height");
    if(heightPtr)
    {
        ss << ", height:" << (uint32_t)heightPtr->Number();
    }

    AMFAnyPtr videocodecidPtr = obj.Property("videocodecid");
    if(videocodecidPtr)
    {
        ss << ", videocodecid:" << (uint32_t)videocodecidPtr->Number();
    }

    AMFAnyPtr frameratePtr = obj.Property("framerate");
    if(frameratePtr)
    {
        ss << ", framerate:" << (uint32_t)frameratePtr->Number();
    }

    AMFAnyPtr videodataratePtr = obj.Property("videodatarate");
    if(videodataratePtr)
    {
        ss << ", videodatarate:" << (uint32_t)videodataratePtr->Number();
    }

    AMFAnyPtr audiosampleratePtr = obj.Property("audiosamplerate");
    if(audiosampleratePtr)
    {
        ss << ", audiosamplerate:" << (uint32_t)audiosampleratePtr->Number();
    }

    AMFAnyPtr audiosamplesizePtr = obj.Property("audiosamplesize");
    if(audiosamplesizePtr)
    {
        ss << ", audiosamplesize:" << (uint32_t)audiosamplesizePtr->Number();
    }

    AMFAnyPtr audiocodecidPtr = obj.Property("audiocodecid");
    if(audiocodecidPtr)
    {
        ss << ", audiocodecid:" << (uint32_t)audiocodecidPtr->Number();
    }

    AMFAnyPtr audiodataratePtr = obj.Property("audiodatarate");
    if(audiodataratePtr)
    {
        ss << " ,audiodatarate:" << (uint32_t)audiodataratePtr->Number();
    }  
    AMFAnyPtr durationPtr = obj.Property("duration");
    if(durationPtr)
    {
        ss << " ,duration:" << (uint32_t)durationPtr->Number();
    }    
    AMFAnyPtr encoderPtr = obj.Property("encoder");
    if(encoderPtr)
    {
        ss << " ,encoder:" << encoderPtr->String();
    } 
    AMFAnyPtr serverPtr = obj.Property("server");
    if(serverPtr)
    {
        ss << " ,server:" << serverPtr->String();
    } 

    LIVE_TRACE << ss.str();   
} 

void CodecHeader::SaveAudioHeader(const PacketPtr &packet)
{
    audio_header_ = packet;
    ++audio_version_; // 新的audio

    audio_header_packets_.emplace_back(packet);

    LIVE_TRACE << "save audio header, audio version:" << audio_version_
                << ", size:" << packet->PacketSize()
                << " elapse:" << TTime::NowMS() - start_timestamp_ << "ms\n";
}

void CodecHeader::SaveVideoHeader(const PacketPtr &packet)
{
    video_header_ = packet;
    ++video_version_; // 新的audio

    video_header_packets_.emplace_back(packet);

    LIVE_TRACE << "save video header, video version:" << video_version_
                << ", size:" << packet->PacketSize()
                << " elapse:" << TTime::NowMS() - start_timestamp_ << "ms\n";
}

/// @brief 根据传入的数据类型解析
/// @param packet 
/// @return 
bool CodecHeader::ParseCodecHeader(const PacketPtr &packet)
{
    if(packet->IsMeta())
    {
        SaveMeta(packet);
        ParseMeta(packet);
    }
    else if(packet->IsAudio())
    {
        SaveAudioHeader(packet);
    }
    else if(packet->IsVideo())
    {
        SaveVideoHeader(packet);
    }
    return true;
}
