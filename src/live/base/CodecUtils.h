/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:38:03
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 22:57:29
 * @FilePath: /tmms/src/live/base/CodecUtils.h
 * @Description:  learn 
 */
#pragma once

#include "mmedia/base/Packet.h"

namespace tmms
{ 
    namespace live
    {
        using namespace tmms::mm;

        class CodecUtils
        {
        public:
            static bool IsCodecHeader(const PacketPtr &packet);
        };
    
        
    } // namespace live
    
} // namespace tmms
