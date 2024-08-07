/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:04:47
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 22:41:36
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.h
 * @Description:  learn 
 */
#pragma once

#include "PlayerUser.h"
#include "mmedia/webrtc/Sdp.h"
#include "mmedia/webrtc/Dtls.h"
#include "mmedia/webrtc/DtlsHandler.h"
#include "mmedia/webrtc/Srtp.h"
#include <string>
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace mm;

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

        private:
            // dtls回调，这里再往上回调
            virtual void OnDtlsSend(const char *data, size_t size, Dtls *dtls) override;
            virtual void OnDtlsHandshakeDone(Dtls *dtls) override;

            static std::string GetUFrag(int size);
            static uint32_t GetSsrc(int size);

            std::string local_ufrag_;
            std::string local_passwd_;
            PacketPtr packet_; // 保存dtls发送的数据
            struct sockaddr_in6 addr_;
            socklen_t addr_len_{sizeof(struct sockaddr_in6)};
            bool dtls_done_{false}; // dtls握手完成标志

            Sdp sdp_;  // 1. 先sdp协商
            Dtls dtls_; // 2. dtls握手，加密udp连接
            Srtp srtp_; // 3. 加密握手之后，启动srtp
        };
        
        
    } // namespace live
    
}