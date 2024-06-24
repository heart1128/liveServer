/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:57:34
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 23:06:17
 * @FilePath: /tmms/src/live/base/CodecUtils.cpp
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
        if(*b == 0)
        {
            return true;
        }
    }

    return false;
}