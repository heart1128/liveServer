/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-29 15:04:56
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 17:37:08
 * @FilePath: /liveServer/src/live/Session.h
 * @Description:  learn 
 */
#pragma once

#include "PlayerUser.h"
#include "base/AppInfo.h"
#include "User.h"
#include <string>
#include <mutex>
#include <unordered_set>
#include <atomic>

namespace tmms
{
    namespace live
    {
        using UserPtr = std::shared_ptr<User>;

        class Session : public std::enable_shared_from_this<Session>
        {
        public:
            explicit Session(const std::string &session_name);
        /// 时间函数
            int32_t ReadyTime() const; // 会话准备好的时间，和流准备好的时间一致
            int64_t SinceStart() const;
            bool IsTimeout();
        /// 用户创建和关闭
            UserPtr CreatePublishUser(const ConnectionPtr &conn,
                            const std::string &session_name,
                            const std::string &param,
                            UserType type);
            UserPtr CreatePlayerUser(const ConnectionPtr &conn,
                            const std::string &session_name,
                            const std::string &param,
                            UserType type);
            void CloseUser(const UserPtr &user);
        /// 用户操作
            void ActiveAllPlayers();
            void AddPlayer(const PlayerUserPtr &user);
            void SetPublisher(UserPtr &user);
        /// 其他函数
            StreamPtr GetStream() ;
            const string &SessionName()const;
            void SetAppInfo(AppInfoPtr &ptr);
            AppInfoPtr &GetAppInfo();
            bool IsPublishing() const; // 判断当前用户有没有在推流
            void Clear(); // 清除session


        private:
            void CloseUserNoLock(const UserPtr &user);

            std::string session_name_;
            std::unordered_set<PlayerUserPtr> players_; // 所有的播放用户
            AppInfoPtr app_info_;
            StreamPtr stream_;  // 用户通过stream进行推流拉流
            UserPtr publisher_; // 推流用户，一个推流对应多个播放
            std::mutex lock_;
            std::atomic<int64_t> player_live_time_;
        };
        
    } // namespace live
    
} // namespace tmms
