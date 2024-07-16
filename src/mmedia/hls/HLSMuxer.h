/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-16 17:55:20
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-16 18:03:31
 * @FilePath: /liveServer/src/mmedia/hls/HLSMuxer.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "Fragment.h"
#include "FragmentWindow.h"
#include "mmedia/mpegts/TsEncoder.h"
#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>
#include <memory>

namespace tmms
{
    namespace mm
    {
        /// @brief 管理切片，切片的策略，进行切片
        class HLSMuxer
        {
        public:
            HLSMuxer(const std::string &name);
            ~HLSMuxer() = default;

            std::string PlayList();
            void OnPacket(PacketPtr &packet);
            FragmentPtr GetFragment(const std::string &name);
            void ParseCodec(FragmentPtr &fragment,PacketPtr &packet);

        private:
            bool IsCodecHeader(const PacketPtr &packet);

            FragmentWindow fragment_window_;
            TsEncoder encoder_;
            FragmentPtr current_fragment_;
            std::string stream_name_;  // 流名作基本文件类型名
            int64_t fragmemt_seq_no_{0};
            int32_t min_fragment_size_{3000}; // 3s，超过这个最小时长，并且有关键帧就能切片
            int32_t max_fragment_size_{12000}; // 12s 没有关键帧超过这个时间说明gop太大，也可以切片
        };
        
        
    } // namespace mm
    
} // namespace tmms
