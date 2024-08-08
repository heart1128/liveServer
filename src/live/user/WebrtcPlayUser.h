/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:04:47
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 16:07:49
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.h
 * @Description:  learn 
 */
#pragma once

#include "PlayerUser.h"
#include "mmedia/webrtc/Sdp.h"
#include "mmedia/webrtc/DtlsCerts.h"
#include <string>
#include <cstdint>

namespace tmms
{
    namespace live
    {
        using namespace mm;

        class WebrtcPlayUser : public PlayerUser
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

        private:
            static std::string GetUFrag(int size);
            static uint32_t GetSsrc(int size);

            std::string local_ufrag_;
            std::string local_passwd_;
            Sdp sdp_;
            DtlsCerts dtls_certs_;
        };
        
        
    } // namespace live
    
}