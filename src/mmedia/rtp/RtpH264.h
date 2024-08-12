#pragma once

#include "Rtp.h"
#include "mmedia/base/AVTypes.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {
        class RtpH264 : public Rtp
        {
        public:
        //  pyload type是sdp协商的
            RtpH264(int32_t pt);
            ~RtpH264() = default;
        
            bool Encode(std::list<SampleBuf> &ins,uint32_t ts,std::list<PacketPtr> &outs) override;
            bool EncodeSpsPps(const std::string &sps,const std::string &pps,std::list<PacketPtr> &outs);
        private:
            bool EncodeSingle(const SampleBuf & buf,bool last, std::list<PacketPtr> &outs);
            bool EncodeFua(const SampleBuf & buf,std::list<PacketPtr> &outs);
        }; 
    } // namespace mm
    
}