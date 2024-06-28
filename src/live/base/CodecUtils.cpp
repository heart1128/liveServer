/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:57:34
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * @LastEditTime: 2024-06-25 20:43:05
 * @FilePath: /liveServer/src/live/base/CodecUtils.cpp
 * @Description:  learn 
 */
#include "CodecUtils.h"

using namespace tmms::live; 

bool CodecUtils::IsCodecHeader(const PacketPtr &packet)
{
    // 第一个字节是FLV
    if(packet->PacketSize() > 1)
    {
        const char *b = packet->Data() + 1;
        // 音频流的AAC第二个字节为0 
        // 视频流的第二个字节为0，就是H.264的
        if(*b == 0)
        {
            return true;
        }
    }
    return false;
}