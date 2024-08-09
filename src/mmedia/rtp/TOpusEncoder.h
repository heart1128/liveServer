#pragma once


#include "opus/opus.h"
#include "mmedia/base/AVTypes.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {
        const int32_t kOpusMaxSize = 32 * 1024;
        class TOpusEncoder
        {
        public:
            TOpusEncoder() = default;
            ~TOpusEncoder();

            bool Init(int sample, int channels);
            bool Encode(SampleBuf &pcm, std::list<PacketPtr> &pkts);
            
        private:
            OpusEncoder *encoder_{nullptr};
            int channels_{2};
            uint8_t pcm_buf_[kOpusMaxSize];
            size_t pcm_bytes_{0};
            size_t frame_bytes_{0};
            int32_t frame_size_{480};
        };

    } // namespace mm
    
} // namespace tmms
