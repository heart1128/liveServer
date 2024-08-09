/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 17:06:19
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-09 17:13:26
 * @FilePath: /liveServer/src/mmedia/rtp/Rtp.h
 * @Description:  learn 
 */
#pragma once

#include "mmedia/base/Packet.h"
#include "mmedia/base/AVTypes.h"

#include <cstdint>
#include <string>
#include <list>

namespace tmms
{
    namespace mm
    {

        enum RtcpPayloadType
        {
            kRtcpPtSR = 200,
            kRtcpPtRR = 201,
            kRtcpPtSdes   = 202,
            kRtcpPtBye    = 203,
            kRtcpPtApp    = 204,
            kRtcpPtRtpfb  = 205,
            kRtcpPtPsfb   = 206,
        };

        class Rtp
        {
        public:
            Rtp(int32_t pt);
            ~Rtp() = default;
        
            virtual bool Encode(std::list<SampleBuf> &ins,uint32_t ts, std::list<PacketPtr> &outs) = 0;
            void SetMarker(bool on);
            void SetSequenceNumber(uint16_t);
            uint16_t SequenceNumber() const;
            void SetTimestamp(uint32_t timestamp);
            uint32_t Timestamp()const;
            void SetSsrc(uint32_t ssrc);
            uint32_t Ssrc()const;
            void SetSample(int32_t s);
        
        protected:
            void EncodeHeader(char *buf);
            int32_t HeaderSize() const;

        // header
            uint32_t version_:2; // 占用两位，不能成员列表初始化了
            uint32_t padding_:1;
            uint32_t extern_:1;
            uint32_t csrc_count_:4;
            uint32_t marker_:1;
            uint32_t payload_type_:7;
            uint32_t sequence_{0};
            uint32_t timestamp_{0};
            uint32_t ssrc_{0};  // 提供信源
            uint32_t sample_{0};
        };
        
        
    } // namespace mm
    
} // namespace tmms
