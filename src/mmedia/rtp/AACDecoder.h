/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 17:35:24
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-09 17:49:03
 * @FilePath: /liveServer/src/mmedia/rtp/AACDecoder.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <string>
#include "mmedia/base/AVTypes.h"
#include "faad.h"
#include "neaacdec.h"

namespace tmms
{
    namespace mm
    {
        class AACDecoder
        {
        public:
            AACDecoder() = default;
            ~AACDecoder() = default;
        
            bool Init(const std::string &config);
            SampleBuf Decode(unsigned char* aac, size_t aac_size);
        
        private:
            NeAACDecHandle handle_;  // 给faad库回调处理
        };
        
    } // namespace mm
    
}