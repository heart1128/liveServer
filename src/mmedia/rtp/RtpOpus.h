#pragma once

#include "Rtp.h"
#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {
        class RtpOpus : public Rtp
        {
        public:
            RtpOpus(int32_t pt);
            ~RtpOpus() = default;

            bool Encode(std::list<SampleBuf> &ins,uint32_t ts, std::list<PacketPtr> &outs) override;
        private:
            
        };
    }
} // namespace tmms

