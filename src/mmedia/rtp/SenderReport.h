/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-18 16:06:05
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-18 16:07:32
 * @FilePath: /liveServer/src/mmedia/rtp/SenderReport.h
 * @Description:  learn 
 */
#pragma once

#include "Rtcp.h"

namespace tmms
{
    namespace mm
    {
        class SenderReport:public Rtcp
        {
        public:
            SenderReport();
            ~SenderReport()=default;

            /**
             * @description: 因为只发送不接收，所以不用decode
             * @param {char} *data
             * @param {size_t} len
             * @return {*}
             */            
            void Decode(const char *data, size_t len) override
            {

            }

            PacketPtr Encode() override; 
            void SetSsrc(uint32_t ssrc);
            void SetNtpTimestamp(uint64_t time);
            void SetRtpTimestamp(uint32_t time);
            void SetSentPacketCount(uint32_t count);
            void SetSentBytes(uint32_t bytes); 
        private:
            uint32_t ssrc_{0};
            uint32_t ntp_mword_{0}; // 64比特，分为两个32比特的部分，用于表示发送端的NTP时间戳（NetworkTimeProtocol）。NTP时间戳用于同步网络时间。
            uint32_t ntp_lword_{0}; // 32比特，用于表示发送端最近发送的RTP包的时间戳。RTP时间戳与RTP数据包的时间戳相关联，用于计算传输间隔和抖动等信息。
            uint32_t rtp_timestamp_{0};
            uint32_t sent_pkt_cnt_{0}; // 32比特，表示发送端自会话开始以来发送的RTP数据包数量。
            uint32_t sent_bytes_{0};  // 32比特，表示发送端自会话开始以来发送的总字节数（包括RTP有效载荷和RTP头部）
        };
    }
}