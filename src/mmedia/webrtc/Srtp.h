/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 15:54:50
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-09 16:29:28
 * @FilePath: /liveServer/src/mmedia/webrtc/Srtp.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <string>
#include "mmedia/base/Packet.h"
#include "srtp2/srtp.h"

namespace tmms
{
    namespace mm
    {
        const int32_t kSrtpMaxBufferSize = 65535;
        class Srtp
        {
        public:
            Srtp() = default;
            ~Srtp() = default;
        
            static bool InitSrtpLibrary();
            bool Init(const std::string &recv_key,const std::string &send_key);
            // 加密解密函数
            PacketPtr RtpProtect(PacketPtr &pkt);
            PacketPtr RtcpProtect( PacketPtr &pkt);
            PacketPtr SrtpUnprotect(PacketPtr &pkt);
            PacketPtr SrtcpUnprotect( PacketPtr &pkt); 
            PacketPtr SrtcpUnprotect(const char *buf,size_t size); 
        private:
            static void OnSrtpEvent(srtp_event_data_t* data);
            srtp_t send_ctx_{nullptr};  // 输入输出上下文
            srtp_t recv_ctx_{nullptr};
            // 读写缓存
            char w_buffer_[kSrtpMaxBufferSize];
            char r_buffer_[kSrtpMaxBufferSize];
        };
    }
}