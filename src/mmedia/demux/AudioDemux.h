/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-05 13:49:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-11 15:21:46
 * @FilePath: /liveServer/src/mmedia/demux/AudioDemux.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-05 13:49:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 16:30:31
 * @FilePath: /liveServer/src/mmedia/demux/AudioDemux.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <list>
#include <cstdint>
#include <string>

#include "mmedia/base/AVTypes.h"

namespace tmms
{
    namespace mm
    {
        class AudioDemux
        {
        public:
            AudioDemux() = default;
            ~AudioDemux() = default;
        
            int32_t OnDemux(const char *data, size_t size, std::list<SampleBuf> &list);
            int32_t GetCodecId() const
            {
                return sound_format_;
            }
            AACObjectType GetObjectType() const
            {
                return aac_object_;
            }
            int32_t GetSampleRateIndex() const
            {
                return aac_sample_rate_;
            }
            uint8_t GetChannel() const
            {
                return aac_channel;
            }

            int32_t GetSampleRate() const;
            const std::string &AACSeqHeader()
            {
                return aac_seq_header_;
            }
        private:
            int32_t DemuxAAC(const char *data, size_t size, std::list<SampleBuf> &list);
            int32_t DemuxMP3(const char *data, size_t size, std::list<SampleBuf> &list);
            int32_t DemuxAACSequenceHeadr(const char *data, int size);

        private:
            int32_t sound_format_;  // mp3 or aac
            int32_t sound_rate_;    
            int32_t sound_size_;
            int32_t sound_type_;
            AACObjectType aac_object_;
            int32_t aac_sample_rate_;
            uint8_t aac_channel;
            bool aac_ok_{false};
            std::string aac_seq_header_;
        };
    } // namespace mm
    
} // namespace tmms
