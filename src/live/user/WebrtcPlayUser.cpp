/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:07:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 16:08:50
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.cpp
 * @Description:  learn
 */
#include "WebrtcPlayUser.h"
#include "live/base/LiveLog.h"

using namespace tmms::live;

WebrtcPlayUser::WebrtcPlayUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
:PlayerUser(ptr, stream, s)
{
}

bool WebrtcPlayUser::PostFrames()
{
    return false;
}

UserType WebrtcPlayUser::GetUserType() const
{
    return UserType::kUserTypePlayerWebRTC;
}
