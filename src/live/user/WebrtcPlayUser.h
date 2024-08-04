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
#include <cstdint>

namespace tmms
{
    namespace live
    {
        class WebrtcPlayUser : public PlayerUser
        {
        public:
            explicit WebrtcPlayUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);
            ~WebrtcPlayUser() = default;

            bool PostFrames() override;
            UserType GetUserType() const override;
        };
        
        
    } // namespace live
    
}