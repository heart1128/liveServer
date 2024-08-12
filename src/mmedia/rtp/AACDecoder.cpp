/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 17:49:01
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 17:46:09
 * @FilePath: /liveServer/src/mmedia/rtp/AACDecoder.cpp
 * @Description:  learn 
 */
#include "AACDecoder.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

/**
 * @description: 读取序列头数据，进行初始化
 * @param {string} &config  aac的序列头数据
 * @return {*}
 */
bool AACDecoder::Init(const std::string &config)
{
    // 创建aac decoder
    handle_ = NeAACDecOpen();
    unsigned long samlperate = 0; // 采样率
    unsigned char channels = 0;   // 声道数
    // 初始化
    auto ret = NeAACDecInit2(handle_, (unsigned char*)config.c_str(), 
                            config.size(), &samlperate, &channels);
    if(ret >= 0)
    {
        WEBRTC_DEBUG << "AACDecoder::Init ok, samplerate: "<< samlperate << " channels:" << channels;
    }
    else
    {
        WEBRTC_ERROR << "AACDecoder::Init failed.ret=" << ret;
        return false;
    }
    return true;
}

/**
 * @description: aac->pcm
 * @param {unsigned char} *aac
 * @param {size_t} aac_size
 * @return {*}
 */
SampleBuf AACDecoder::Decode(unsigned char *aac, size_t aac_size)
{
    NeAACDecFrameInfo frame_info;
    // aac解码成frame（pcm格式），内部使用ffmpeg实现
    char *data = (char*)NeAACDecDecode(handle_, &frame_info, aac, aac_size);
    if(data && frame_info.samples > 0 && frame_info.error == 0)
    {
        // 一帧的大小就是单通道采样的大小*通道数
        int32_t bytes = frame_info.samples * frame_info.channels;
        return SampleBuf(data, bytes);
    }
    else if(frame_info.error > 0)
    {
        WEBRTC_ERROR << "decode failed.error:" << frame_info.error;
    }
    // 没有音频数据帧
    return SampleBuf(nullptr, 0);
}
