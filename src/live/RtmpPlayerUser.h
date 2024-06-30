#pragma once

#include "PlayerUser.h"

namespace tmms
{
    namespace live
    {
        class RtmpPlayerUser : public PlayerUser
        {
        public:
            // using PlayerUser::User;   // 使用c++11的委托构造
            explicit RtmpPlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);
        
        public:
            bool PostFrames();
            UserType GetUserType() const;
        
        private:
            using User::SetUserType; // 同样使用父类的
            // 发送头和meta
            bool PushFrame(PacketPtr &packet, bool is_header);
            // 发送数据帧
            bool PushFrame(std::vector<PacketPtr> &list);
        };
        
    
        
    } // namespace llive
    
} // namespace tmms
