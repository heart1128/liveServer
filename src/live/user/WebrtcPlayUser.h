/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:04:47
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:11:16
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.h
 * @Description:  learn 
 */
#pragma once

#include "PlayerUser.h"
#include "mmedia/webrtc/Sdp.h"
#include "mmedia/webrtc/Dtls.h"
#include "mmedia/webrtc/DtlsHandler.h"
#include "mmedia/webrtc/Srtp.h"
#include "mmedia/rtp/RtpMuxer.h"
#include "network/base/InetAddress.h"
#include <string>
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace mm;

        using SockAddrIn6Ptr = std::shared_ptr<struct sockaddr_in6>;

        class WebrtcPlayUser : public PlayerUser, public DtlsHandler
        {
        public:
            explicit WebrtcPlayUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);

            bool PostFrames() override;
            UserType GetUserType() const override;

            bool ProcessOfferSdp(const std::string &sdp);
            const std::string &LocalUFrag() const;
            const std::string &LocalPasswd() const;
            const std::string &RemoteUFrag() const;
            
            std::string BuildAnswerSdp();
            void SetConnection(const ConnectionPtr &conn) override;
            void OnDtlsRecv(const char* buf, size_t size);

            SockAddrIn6Ptr GetSockAddr() const
            {
                return addr_;
            }
            void SetSockAddr(const base::InetAddress &addr)
            {
                if(!addr_)
                {
                    addr_ = std::make_shared<struct sockaddr_in6>();
                }
                addr.GetSockAddr((struct sockaddr*)addr_.get());
            }
            
            void OnRtcp(const char* buf, size_t size);

        private:
            // dtls回调，这里再往上回调
            virtual void OnDtlsSend(const char *data, size_t size, Dtls *dtls) override;
            virtual void OnDtlsHandshakeDone(Dtls *dtls) override;

            static std::string GetUFrag(int size);
            static uint32_t GetSsrc(int size);
            void SendSR(bool is_video);
            void CheckSR();

            void AddVideo(const PacketPtr &pkt);
            void AddAudio(const PacketPtr &pkt);
            PacketPtr GetVideo(int idx);
            PacketPtr GetAudio(int idx); 
            void ProcessRtpfb(const char *buf,size_t size);

            std::string local_ufrag_;
            std::string local_passwd_;
            PacketPtr packet_; // 保存dtls发送的数据
            SockAddrIn6Ptr addr_;
            socklen_t addr_len_{sizeof(struct sockaddr_in6)};
            bool dtls_done_{false}; // dtls握手完成标志

            Sdp sdp_;  // 1. 先sdp协商
            Dtls dtls_; // 2. dtls握手，加密udp连接
            Srtp srtp_; // 3. 加密握手之后，启动srtp

            RtpMuxer rtp_muxer_;
            bool got_key_frame_{false};

            // RTCP的SR报文信息统计
            uint32_t video_out_bytes_{0};
            uint32_t audio_out_bytes_{0};
            uint32_t video_out_pkts_count_{0};
            uint32_t audio_out_pkts_count_{0};
            uint64_t video_sr_timestamp_{0};  // 上一次sr的timestamp
            uint64_t audio_sr_timestamp_{0};

            std::mutex queue_lock_;
            std::vector<PacketPtr> video_queue_;
            std::vector<PacketPtr> audio_queue_;
        };
        
        
    } // namespace live
    
}