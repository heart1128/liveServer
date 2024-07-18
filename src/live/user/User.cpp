#include "User.h"
#include "base/TTime.h"
#include "live/Stream.h"

using namespace tmms::live;

User::User(const ConnectionPtr &ptr, const StreamPtr &stream,  const SessionPtr &s)
:connection_(ptr), stream_(stream), session_(s)
{
    start_timestamp_ = TTime::NowMS();
    user_id_ = ptr->PeerAddr().ToIpPort();
}

User::~User()
{
}

const string &User::DomainName() const
{
    return domain_name_;
}

void User::SetDomainName(const string &domain)
{
    domain_name_ = domain;
}

const string &User::AppName() const
{
    return app_name_;
}

void User::SetAppName(const string &domain)
{
    app_name_ = domain;
}

const string &User::StreamName() const
{
    return stream_name_;
}

void User::SetStreamName(const string &domain)
{
    stream_name_ = domain;
}

const string &User::Param() const
{
    return param_;
}

void User::SetParam(const string &domain)
{
    param_ = domain;
}

const AppInfoPtr &User::GetAppInfo() const
{
    return app_info_;
}

void User::SetAppInfo(const AppInfoPtr &info)
{
    app_info_ = info;
}

UserType User::GetUserType() const
{
    return type_;
}

void User::SetUserType(UserType t)
{
    type_ = t;
}

UserProtocol User::GetUserProtocol() const
{
    return protocol_;
}

void User::SetUserProtocol(UserProtocol p)
{
    protocol_ = p;
}

void User::Close()
{
    if(connection_)
    {
        // 清除连接，调用close回调函数通知
        connection_->ForceClose();
    }
}

ConnectionPtr User::getConnection()
{
    return connection_;
}

uint64_t User::ElaspsedTime()
{
    return TTime::NowMS() - start_timestamp_;
}

void User::Avtive()
{
    if(connection_)
    {
        connection_->Active();
    }
}

void User::Deactive()
{
    if(connection_)
    {
        connection_->Deactive();
    }
}
  