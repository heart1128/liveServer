/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 10:38:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-11 16:19:45
 * @FilePath: /liveServer/src/mmedia/mpegts/TsTool.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
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
            static void WritePts(uint8_t *q, int fourbits, int64_t pts);
            static bool IsCodecHeader(const PacketPtr &packet);
        };
    }
}