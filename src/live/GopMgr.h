/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 16:38:29
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 10:39:33
 * @FilePath: /liveServer/src/live/GopMgr.h
 * @Description:  learn 
 */
#pragma once

#include "mmedia/base/Packet.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;


        struct GopItemInfo
        {
            int32_t index;      // i帧的index
            int64_t timestamp;  // i帧的timestamp
            GopItemInfo(int32_t i, int64_t t)
            :index(i), timestamp(t)
            {}
        };

        class GopMgr
        {
        public:
            GopMgr(/* args */);
            ~GopMgr();

            void AddFrame(const PacketPtr& packet); // 有一个视频帧就添加到gops
            int32_t MaxGopLength() const;           // 最大的gop len
            size_t GopSize() const;
            int GetGopByLatency(int content_latency, int & latency) const;
            void ClearExpriedGop(int min_idx);
            void PrintAllGop();
            int64_t LastestTimeStamp() const
            {
                return lastest_timestamp_;
            }
        
        private:
            std::vector<GopItemInfo> gops_;
            int32_t gop_length_{0};
            int32_t max_gop_length_{0};
            int32_t gop_numbers_{0};
            int32_t total_gop_length_{0}; // 有多少个帧
            int64_t lastest_timestamp_{0};
        };
    
        
    } // namespace live
    
} // namespace tmms
