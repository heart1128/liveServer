
#pragma once

#include "StreamWriter.h"
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        
        // 后续实现不同写
        class TestStreamWriter : public StreamWriter
        {
        public:
            TestStreamWriter();
            ~TestStreamWriter();
            
            virtual void AppendTimeStamp(int64_t pts) override {};
            virtual int32_t Write(void* buf, uint32_t size) override;
            virtual char* Data() override { return nullptr; };
            virtual int Size() override { return 0; };
        
        private:
            int fd_{-1};
        }; 

    }
}