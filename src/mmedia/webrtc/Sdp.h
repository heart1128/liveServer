/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 18:51:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-05 17:56:55
 * @FilePath: /liveServer/src/mmedia/webrtc/Sdp.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include <cstdint>
#include <string>

namespace tmms
{
    namespace mm
    {
        class Sdp
        {
        public:
            Sdp() = default;
            ~Sdp() = default;

            bool Decode(const std::string &sdp);
            /**
             * @description: 获取远端的ice用户名
             */            
            const std::string &GetRemoteUFrag() const;

            /**
             * @description: 获取远端的指纹
             */            
            const std::string &GetFingerprint() const;

            int32_t GetVideoPayloadType() const;
            int32_t GetAudioPayloadType() const;

            void SetFingerprint(const std::string &fp);
            void SetStreamName(const std::string &name);
            void SetLocalUFrag(const std::string &frag);
            void SetLocalPasswd(const std::string &pwd);
            void SetServerPort(uint16_t port);
            void SetServerAddr(const std::string &addr);
            void SetVideoSsrc(uint32_t ssrc);
            void SetAudioSsrc(int32_t ssrc);
            const std::string &GetLocalPasswd() const;
            const std::string &GetLocalUFrag() const;
            uint32_t VideoSsrc() const;
            uint32_t AudioSsrc() const;
            std::string Encode();
        
        private:
            int32_t audio_payload_type_{-1};
            int32_t video_payload_type_{-1};
            std::string remote_ufrag_;  // 对端的ice用户名
            std::string remote_passwd_;
            std::string local_ufrag_;
            std::string local_passwd_;
            std::string fingerprint_;

            // 组装
            int32_t video_ssrc_{0};
            int32_t audio_ssrc_{0};
            std::string server_addr_;
            int16_t server_port_{0};
            std::string stream_name_;
        };
        
    } // namespace mm
    
}