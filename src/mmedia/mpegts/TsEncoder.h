#pragma once

#include "mmedia/base/AVTypes.h"
#include "StreamWriter.h"
#include "AudioEncoder.h"
#include "VideoEncoder.h"
#include "PatWriter.h"
#include "PmtWriter.h"
#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {
        class TsEncoder
        {
        public:
            TsEncoder(/* args */) = default;
            ~TsEncoder() = default;

            int32_t Encode(StreamWriter* writer, PacketPtr &data,int64_t dts);
            void SetStreamType(StreamWriter * w,VideoCodecID vc, AudioCodecID ac);
            int32_t WritePatPmt(StreamWriter * w);

        private:
            PatWriter pat_writer_;  // PSI
            PmtWriter pmt_writer_;
            AudioEncoder audio_encoder_;  // PES
            VideoEncoder video_encoder_;
            TsStreamType video_type_{kTsStreamReserved};
            TsStreamType audio_type_{kTsStreamReserved};
            uint16_t video_pid_{0xe000};  // pes中的pid
            uint16_t audio_pid_{0xe000};
        };
        
    } // namespace mm
    
} // namespace tmms