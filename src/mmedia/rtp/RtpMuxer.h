/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-12 16:16:21
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-12 16:53:01
 * @FilePath: /liveServer/src/mmedia/rtp/RtpMuxer.h
 * @Description:  learn 
 */
#pragma once

#include "mmedia/base/AVTypes.h"
#include "mmedia/base/Packet.h"
#include "mmedia/demux/AudioDemux.h"
#include "mmedia/demux/VideoDemux.h"
#include "TOpusEncoder.h"
#include "AACDecoder.h"
#include "Rtp.h"
#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {
        /**
         * @description: 整合音视频，复用器
         */        
        class RtpMuxer
        {
        public:
            RtpMuxer() = default;
            ~RtpMuxer() = default;

            bool Init(int32_t vpt,int32_t apt,uint32_t vssrc,uint32_t assrc);
            int32_t EncodeVideo(PacketPtr &pkt, std::list<PacketPtr>&rtp_pkts, uint32_t timestamp);
            int32_t EncodeAudio(PacketPtr &pkt, std::list<PacketPtr> &rtp_pkts, uint32_t timestamp);
            uint32_t VideoTimestamp() const;
            uint32_t AudioTimestamp() const;
            uint32_t VideoSsrc() const;
            uint32_t AudioSsrc() const;
        private:
            std::shared_ptr<Rtp> video_rtp_;
            std::shared_ptr<Rtp> audio_rtp_;
            std::shared_ptr<VideoDemux> video_demux_;
            std::shared_ptr<AudioDemux> audio_demux_;
            std::shared_ptr<TOpusEncoder> opus_encoder_;
            std::shared_ptr<AACDecoder> aac_decoder_;
            bool sent_sps_pps_{false};
        };

        
    }
}