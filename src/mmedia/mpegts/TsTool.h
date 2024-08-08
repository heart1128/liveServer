/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 10:38:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 17:50:04
 * @FilePath: /liveServer/src/mmedia/mpegts/TsTool.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include "mmedia/base/Packet.h"

namespace tmms
{
    namespace mm
    {
        class TsTool
        {
        public:
            static std::string HexString(uint32_t s);
            static uint32_t CRC32(const void* buf, int size);
            static uint32_t CRC32Ieee(const void* buf, int size);
            static void WritePts(uint8_t *q, int fourbits, int64_t pts);
            static bool IsCodecHeader(const PacketPtr &packet);
        };
    }
}