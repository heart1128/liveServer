/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-18 16:50:19
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:39:01
 * @FilePath: /liveServer/src/mmedia/rtp/Rtpfb.h
 * @Description:  learn 
 */
#pragma once

#include "Rtcp.h"
#include <cstdint>
#include <set>

namespace tmms
{
    namespace mm
    {
        /**
         * @description: 序列号绕了一圈，这里判断新旧序列号
         * @return {*}
         */        
        struct SeqCmp
        {
            int16_t uint16_t_distance(uint16_t prev, uint16_t current) const
            {
                return (int16_t)(current - prev);
            }

            bool is_uint16_t_newer(uint16_t prev, uint16_t current) const
            {
                return uint16_t_distance(prev, current) > 0;
            }

            bool operator()(uint16_t prev, uint16_t current) const
            {
                return is_uint16_t_newer(prev,current);
            }

        };
        
        class Rtpfb:public Rtcp
        {
        public:
            Rtpfb();
            ~Rtpfb() = default;

            uint32_t Ssrc() const;
            uint32_t MediaSsrc() const;
            const std::set<uint16_t,SeqCmp> &LostSeqs()const;
            size_t DecodeNack(const char *data, size_t len);
            void Decode(const char *data, size_t len) override;
            /**
             * @description: 因为服务端只需要处理NACK包（客户端丢失的包序号），然后发送丢失的包序号即可
             * @return {*}
             */            
            PacketPtr Encode() override;
            void Print();
        private:
            uint32_t ssrc_{0};
            uint32_t media_ssrc_{0};
            std::set<uint16_t,SeqCmp> lost_seqs_;
        };
    }
}