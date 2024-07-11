#pragma once

#include "mmedia/base/AVTypes.h"
#include "StreamWriter.h"
#include "mmedia/demux/AudioDemux.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <list>

namespace tmms
{
    namespace mm
    {
        class AudioEncoder
        {
        public:
            AudioEncoder(/* args */) = default;
            ~AudioEncoder() = default;
        
        public:
            int32_t EncodeAudio(StreamWriter * writer, PacketPtr &data, int64_t dts);

            /**
             * @description: 因为rtmp用flv组装数据，flv使用的AAC音频格式用的是个ADIF封装，而MPEG-TS用的是ADTS封装
             *              所以需要使用AudioDemux.h进行flv的音频解码拿到原始音频数据，然后使用encoder组装成ADTS到TS中
             * @param {StreamWriter} *writer
             * @param {list<SampleBuf>} &sample_list
             * @param {int64_t} pts
             * @return 正确返回0，失败返回-1
            **/            
            int32_t EncodeAAC(StreamWriter *writer, std::list<SampleBuf> &sample_list, int64_t pts);

            /**
             * @description: MP3格式FLV和MPEG-TS都是一样的格式，不用转换
             * @return {*}
            **/            
            int32_t EncodeMP3(StreamWriter *write, std::list<SampleBuf> &sample_list, int64_t pts);

            /**
             * @description: 组装 TS header + Ts header + audio data = 188bytes
             * @param {StreamWriter} *write
             * @param {list<SampleBuf>} &result
             * @param {int} payload_size
             * @param {int64_t} dts
             * @return {*}
            **/            
            int32_t WriteAudioPes(StreamWriter *write, std::list<SampleBuf> &result, int payload_size, int64_t dts); 

            void SetPid(uint16_t pid);
            void SetStreamType(TsStreamType type);
        private:
            uint16_t pid_{0xe000};
            TsStreamType type_{kTsStreamReserved};
            int8_t cc_{-1};
            AudioDemux demux_;
        };
        
    } // namespace mm
    
} // namespace tmms
