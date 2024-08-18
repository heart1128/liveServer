/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-18 15:47:01
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-18 15:48:24
 * @FilePath: /liveServer/src/mmedia/rtp/Rtcp.h
 * @Description:  learn 
 */
#pragma once

#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>

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

        class Rtcp
        {
        public:
            Rtcp(int32_t pt);
            ~Rtcp()=default;

            virtual void Decode(const char *data, size_t len) = 0;
            virtual PacketPtr Encode() = 0; 

        protected:
            int32_t DecodeHeader(const char *data, size_t len);
            int32_t EncodeHeader(char *buf);         
            void SetRC(int count);
            int  RC() const;
            void SetPayloadType(int type);
            int PayloadType() const;
            void SetPadding(bool pad);
            bool Padding() const;
            void SetPayloadLength(uint32_t size);
            uint32_t PayloadLength() const;


            uint32_t version_:2;
            uint32_t padding_:1;
            uint32_t rc_:5;    // rtcp报文中包含的接受者报告块的数量
            uint32_t payload_type_:8;
            uint32_t pl_len_:16;
        };
    }
}