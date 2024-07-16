/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-11 17:16:43
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-15 15:37:43
 * @FilePath: /liveServer/src/mmedia/mpegts/VideoEncoder.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "StreamWriter.h"
#include "mmedia/base/Packet.h"
#include "mmedia/base/AVTypes.h"
#include "mmedia/demux/VideoDemux.h"
#include <cstdint>
#include <list>

namespace tmms
{
    namespace mm
    {
        class VideoEncoder
        {
        public:
            VideoEncoder() = default;
            ~VideoEncoder() = default;

        public:
            int32_t EncodeVideo(StreamWriter* writer, bool key,PacketPtr &data,int64_t dts);
            void SetPid(uint16_t pid);
            void SetStreamType (TsStreamType type);

        private:

            /**
             * @description: 从flv解析出来的video raw NULA，组装成start NULA格式。
             *          注意的是，SPS和PPS在一次包中发送一次，并且是在IDR帧之前
             * @param {int64_t} pts
             * @return {*}
            **/            
            int32_t EncodeAvc(StreamWriter* writer,std::list<SampleBuf> &sample_list,bool key,int64_t pts);

            /**
             * @description: 从flv解析出来的NULA，需要变成annbex格式，也就是AVVC前面变成startCode不是长度
             * @param {list<SampleBuf>} &sample_list
             * @return {*}
            **/            
            int32_t AvcInsertStartCode(std::list<SampleBuf> &sample_list,bool&);

            /**
             * @description: TS header + PES packet + NULA
             * @return {*}
            **/            
            int32_t WriteVideoPes(StreamWriter* writer,std::list<SampleBuf> &result, int payload_size,int64_t pts, int64_t dts, bool key);

        private:
            uint16_t pid_{0xe000};
            TsStreamType type_{kTsStreamReserved};
            int8_t cc_{-1};
            bool startcode_inserted_{false};  // 转换成axxbe的是否标志是否插入了start code
            bool sps_pps_appended_{false};
            VideoDemux demux_;
        };
    
        
    } // namespace mm
    
} // namespace tmms
