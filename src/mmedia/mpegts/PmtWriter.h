#pragma once

#include "PSIWriter.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace tmms
{
    namespace mm
    {
        /**
         * @description: 节目信息
        **/        
        struct ProgramInfo
        {
            uint8_t stream_type; // 8bits
            int16_t elementary_pid; // 13bits
        };
        using ProgramInfoPtr = std::shared_ptr<ProgramInfo>;
        

        /**
         * @description: 节目信息表
        **/        
        class PmtWriter : public PSIWriter
        {
        public:
            PmtWriter(/* args */);
            ~PmtWriter() = default;

            void Writepmt(StreamWriter * w);
            void AddProgramInfo(ProgramInfoPtr &program);
            void SetPcrPid(int32_t pid);
        
        private:
            uint16_t pcr_pid_{0xe000}; // 13 bits
            std::vector<ProgramInfoPtr> programs_;  // 节目列表
        };
    
        
    } // namespace mm
    
} // namespace tmms
