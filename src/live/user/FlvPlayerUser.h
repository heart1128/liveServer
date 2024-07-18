/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-04 14:55:26
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-17 17:10:41
 * @FilePath: /liveServer/src/live/user/FlvPlayerUser.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-04 14:55:26
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-04 14:58:37
 * @FilePath: /liveServer/src/live/FlvPlayerUser.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include "live/user/PlayerUser.h"

namespace tmms
{
    namespace live
    {
        class FlvPlayerUser : public PlayerUser
        {
        public:
            // using PlayerUser::User;   // 使用c++11的委托构造
            explicit FlvPlayerUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);
        
        public:
            bool PostFrames();
            UserType GetUserType() const;
        
        private:
            using User::SetUserType; // 同样使用父类的
            // 发送头和meta
            bool PushFrame(PacketPtr &packet, bool is_header);
            // 发送数据帧
            bool PushFrame(std::vector<PacketPtr> &list);

            void PushFlvHttpHeader();
            bool http_header_send{false}; // http头是否已发送
        };
        
    
        
    } // namespace llive
    
} // namespace tmms
