#pragma once

#include "PSIWriter.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        /**
         * @description: PAT是节目关联表，列出TS内所有节目
         * 
         *          PID固定为0x0000
         *          table_id 固定为0x00
         *      每个节目由一个16比特的字段program_number指定
         *      每个program_number都有一个对应的PID，用来指定该节目的PMT。
         * PAT中不包含节目信息时，program_number为0x0000，则应从NIT（PID为0x0010）获取节目信息
        **/        
        class PatWriter : public PSIWriter
        {
        public:
            PatWriter(/* args */);
            ~PatWriter() = default;
        
            void WritePat(StreamWriter* w);
        
        private:
            uint16_t progoram_number_{0x0001};  // 指定一个节目
            uint16_t pmt_pid_{0x1001};          // 指定PMT(节目对应的流)
            uint16_t transport_stream_id_{0x0001}; // 传输流ID
        };
        
    } // namespace mm
    
} // namespace tmms
