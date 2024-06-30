/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-29 15:09:16
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-30 10:21:14
 * @FilePath: /liveServer/src/live/Session.cpp
 * @Description:  learn 
 */
#include "Session.h"
#include "Stream.h"
#include "base/TTime.h"
#include "base/AppInfo.h"
#include "live/base/LiveLog.h"
#include "base/StringUtils.h"
#include "RtmpPlayerUser.h"

using namespace tmms::live;

namespace 
{
    static UserPtr user_null;
} // namespace 


Session::Session(const std::string &session_name)
:session_name_(session_name)
{
    // 流创建
    stream_ = std::make_shared<Stream>(session_name, *this);
    player_live_time_ = TTime::NowMS();
}

/// @brief 会话准备好的时间，和流的时间一致
/// @return 
int32_t Session::ReadyTime() const
{
    return stream_->ReadyTime();
}

int64_t Session::SinceStart() const
{
    return stream_->SinceStart();
}

bool Session::IsTimeout()
{
    // 流超时
    if(stream_->Timeout())
    {
        return true;
    }

    // 会话开始到现在的时间
    auto idle = TTime::NowMS() - player_live_time_;
    // 没有用户的时间达到配置的时间，超时
    if(players_.empty() && idle > app_info_->stream_idle_time)
    {
       return true; 
    }

    return false;
}

UserPtr Session::CreatePublishUser(const ConnectionPtr &conn, 
                        const std::string &session_name, 
                        const std::string &param, 
                        UserType type)
{
    // 流和创建的用户session不一致，就是不能创建的
    if(session_name != session_name_)
    {
        LIVE_ERROR << "create publish user failed. Invalid session name:" << session_name;
        return user_null;
    }

    // session_name = domain/app/stream_name
    auto list = base::StringUtils::SplitString(session_name, "/");
    if(list.size() != 3)
    {
        LIVE_ERROR << "create publish user failed. Invalid session name:" << session_name;
        return user_null;
    }

    UserPtr user = std::make_shared<User>(conn, stream_, shared_from_this());
    user->SetAppInfo(app_info_);;
    user->SetDomainName(list[0]);
    user->SetAppName(list[1]);
    user->SetStreamName(list[2]);
    user->SetParam(param);
    user->SetUserType(type);
    conn->SetContext(kUserContext, user);
    return user;
}

UserPtr Session::CreatePlayerUser(const ConnectionPtr &conn,
                         const std::string &session_name, 
                         const std::string &param, 
                         UserType type)
{
    // 流和创建的用户session不一致，就是不能创建的
    if(session_name != session_name_)
    {
        LIVE_ERROR << "create publish user failed. Invalid session name:" << session_name;
        return user_null;
    }

    // session_name = domain/app/stream_name
    auto list = base::StringUtils::SplitString(session_name, "/");
    if(list.size() != 3)
    {
        LIVE_ERROR << "create publish user failed. Invalid session name:" << session_name;
        return user_null;
    }
    
    PlayerUserPtr user;
    if(type == UserType::kUserTypePlayerRtmp)
    {
        user = std::make_shared<RtmpPlayerUser>(conn, stream_, shared_from_this());
    }
    user->SetAppInfo(app_info_);;
    user->SetDomainName(list[0]);
    user->SetAppName(list[1]);
    user->SetStreamName(list[2]);
    user->SetParam(param);
    user->SetUserType(type);
    conn->SetContext(kUserContext, user);
    return user;
}

/// @brief 在session中删除一个用户，清除指针
/// @param user 
void Session::CloseUser(const UserPtr &user)
{
    // atomic变量，交换新的值， 返回原来的值
    if(!user->destroyed_.exchange(true))
    {
        {
            std::lock_guard<std::mutex> lk(lock_);
            if(user->GetUserType() <= UserType::kUserTypePlayerWebRTC)
            {
                // 销毁publisher用户
                if(publisher_)
                {
                    LIVE_DEBUG << "remove publisher, session name: "<< session_name_
                            << ", user: " << user->UserId()
                            << ", elapsed:" << user->ElaspsedTime()
                            << ", ReadyTime:" << ReadyTime()
                            << ", stream time:" << SinceStart();
                    publisher_.reset();
                }
            }
            else
            {
                // 销毁player用户
                players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));
                player_live_time_ = tmms::base::TTime::NowMS();
                LIVE_DEBUG << "remove player, session name: "<< session_name_
                    << ", user: " << user->UserId()
                    << ", elapsed:" << user->ElaspsedTime()
                    << ", ReadyTime:" << ReadyTime()
                    << ", stream time:" << SinceStart();
            }
        }
    }
}

void Session::ActiveAllPlayers()
{
    // 用户的容器是所有线程共用的
    std::lock_guard<std::mutex> lk(lock_);
    for(auto const &u : players_)
    {
        u->Avtive();
    }
}

void Session::AddPlayer(const PlayerUserPtr &user)
{
    {
        std::lock_guard<std::mutex> lk(lock_);  
        players_.insert(user);
    }

    LIVE_DEBUG << " add player, session name: "<< session_name_ << ", user:" << user->UserId();

    // 如果此时publisher还没有，就是一个回源的动作
    // 在这rtmp服务器没有流媒体内容，需要从别的服务器找到Publisher
    if(!publisher_)
    {
        // TODO 回源实现
    }
    user->Avtive();
}

void Session::SetPublisher(UserPtr &user)
{
    std::lock_guard<std::mutex> lk(lock_); 
    if(publisher_ == user)
    {
        return;
    }

    // 不为空且没有销毁
    if(publisher_ && !publisher_->destroyed_.exchange(true))
    {
        // 关闭原来的，设置新的推流
        publisher_->destroyed_.exchange(true);
        publisher_->Close();
    }

    // 如果为空，直接赋值
    publisher_ = user;
}

StreamPtr Session::GetStream()
{
    return stream_;
}

const string &Session::SessionName() const
{
    return session_name_;
}

void Session::SetAppInfo(AppInfoPtr &ptr)
{
    app_info_ = ptr;
}

AppInfoPtr &Session::GetAppInfo()
{
    return app_info_;
}

bool Session::IsPublishing() const
{
    // !publisher_不是空指针就是false
    // !!publisher就是true
    return !!publisher_;
}

void Session::Clear()
{
    std::lock_guard<std::mutex> lk(lock_); 
    if(publisher_)
    {
        CloseUserNoLock(publisher_);
    }
    for(auto const & p : players_)
    {
        CloseUserNoLock(std::dynamic_pointer_cast<User>(p));
    }
    players_.clear();
}

/// @brief 这个是不带锁的，因为上面带锁的被Clear()调用就重入锁造成死锁了
/// @param user 
void Session::CloseUserNoLock(const UserPtr &user)
{
    // atomic变量，交换新的值， 返回原来的值
    if(!user->destroyed_.exchange(true))
    {
        if(user->GetUserType() <= UserType::kUserTypePlayerWebRTC)
        {
            // 销毁publisher用户
            if(publisher_)
            {
                LIVE_DEBUG << "remove publisher, session name: "<< session_name_
                        << ", user: " << user->UserId()
                        << ", elapsed:" << user->ElaspsedTime()
                        << ", ReadyTime:" << ReadyTime()
                        << ", stream time:" << SinceStart();

                user->Close();
                publisher_.reset();
            }
        }
        else
        {
            LIVE_DEBUG << "remove player, session name: "<< session_name_
                << ", user: " << user->UserId()
                << ", elapsed:" << user->ElaspsedTime()
                << ", ReadyTime:" << ReadyTime()
                << ", stream time:" << SinceStart();
            // 销毁player用户, publiser也可能是player
            players_.erase(std::dynamic_pointer_cast<PlayerUser>(user));
            user->Close();
            player_live_time_ = tmms::base::TTime::NowMS();
        }
    }
}
