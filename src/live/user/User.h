/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 17:43:07
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 18:38:36
 * @FilePath: /liveServer/src/live/user/User.h
 * @Description:  learn 
 */
#pragma once

#include "network/net/Connection.h"
#include "base/AppInfo.h"
#include <memory>
#include <cstdint>
#include <string>

namespace tmms
{
    namespace live
    {
        using namespace tmms::network;
        using namespace tmms::base;
        using std::string;

        enum class UserType
        {
            kUserTypePublishRtmp = 0,
            kUserTypePublishMpegts ,
            kUserTypePublishPav,
            kUserTypePublishWebRtc ,
            kUserTypePlayerPav ,
            kUserTypePlayerFlv ,
            kUserTypePlayerHls ,
            kUserTypePlayerRtmp ,
            kUserTypePlayerWebRTC ,
            kUserTypeUnknowed = 255,
        };

        // c++11 class，枚举有类的作用域和属性
        enum class UserProtocol
        {
            kUserProtocolHttp = 0,
            kUserProtocolHttps ,
            kUserProtocolQuic ,
            kUserProtocolRtsp ,
            kUserProtocolWebRTC ,
            kUserProtocolUdp ,
            kUserProtocolUnknowed = 255
        };

        class Stream;
        using StreamPtr = std::shared_ptr<Stream>;
        class Session; // session要包含user头文件，这里如果加入了session头文件就循环包含了,用前向声明解决
        using SessionPtr = std::shared_ptr<Session>;
        
        class User : public std::enable_shared_from_this<User>
        {
        public:
            friend class Session;
            explicit User(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s);
            virtual ~User();
        
        public:
            const string &DomainName() const ;
            void SetDomainName(const string &domain);
            const string &AppName() const ;
            void SetAppName(const string &domain);
            const string &StreamName() const ;
            void SetStreamName(const string &domain);
            const string &Param() const ;
            void SetParam(const string &domain);   
            const AppInfoPtr &GetAppInfo()const;
            void SetAppInfo(const AppInfoPtr &info);
            virtual UserType GetUserType() const;
            void SetUserType(UserType t);
            virtual UserProtocol GetUserProtocol() const ;
            void SetUserProtocol(UserProtocol p) ;
        
        public:
            void Close();
            ConnectionPtr getConnection();
            uint64_t ElaspsedTime(); // 自用户创建开始经历了多少时间
            virtual void SetConnection(const ConnectionPtr &conn);
            void Avtive();
            void Deactive();
            const std::string &UserId() const
            {
                return user_id_;
            }
            SessionPtr GetSession() const
            {
                return session_;
            }
            StreamPtr GetStream() const
            {
                return stream_;
            }
        
        protected:
            ConnectionPtr connection_; // 用户的active通过conn放在loop中之执行
            StreamPtr stream_;  // 用户的推流，拉流的流
            string domain_name_;
            string app_name_;
            string stream_name_;
            string param_;
            string user_id_;    // 标识不同的user，用ip:port标识
            AppInfoPtr app_info_;
            int64_t start_timestamp_{0};
            UserType type_{UserType::kUserTypeUnknowed};
            UserProtocol protocol_{UserProtocol::kUserProtocolUnknowed};
            std::atomic_bool destroyed_{false}; // 用户有没有被销毁
            SessionPtr session_;
        };
    
        
    } // namespace live
    
} // namespace tmms
