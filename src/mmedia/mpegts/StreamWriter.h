/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 10:43:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-16 15:47:20
 * @FilePath: /liveServer/src/mmedia/mpegts/StreamWriter.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include <cstdint>

namespace tmms
{
    namespace mm
    {
        const int32_t kSectionMaxSize = 1020;
        
        // 后续实现不同写
        class StreamWriter
        {
        public:
            StreamWriter(){}
            virtual ~StreamWriter(){}
            
            virtual void AppendTimeStamp(int64_t dts) = 0;
            virtual int32_t Write(void* buf, uint32_t size) = 0;
            virtual char* Data() = 0;
            virtual int Size() = 0;
        }; 

    }
}