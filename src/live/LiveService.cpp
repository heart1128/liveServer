/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-29 16:34:10
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-06 11:43:19
 * @FilePath: /liveServer/src/live/LiveService.cpp
 * @Description:  learn 
 */
#include "LiveService.h"
#include "base/StringUtils.h"
#include "live/base/LiveLog.h"
#include "base/Config.h"
#include "Session.h"
#include "base/TTime.h"
#include "mmedia/rtmp/RtmpServer.h"
#include "mmedia/http/HttpServer.h"
#include "mmedia/http/HttpRequest.h"
#include "mmedia/http/HttpUtils.h"
#include "mmedia/http/HttpContext.h"
#include "network/DnsServer.h"
#include "mmedia/flv/FlvContext.h"

using namespace tmms::live;
using namespace tmms::mm;

namespace
{
    static SessionPtr session_null;
}

/// @brief 先创建过的流，如果没找到就看配置文件有没有配置，最后全是配置文件里的流
/// @param session_name 
/// @return 
SessionPtr LiveService::CreateSession(const std::string &session_name)
{
    std::lock_guard<std::mutex> lk(lock_);
    // 1. 查找到直接返回
    auto iter = sessions_.find(session_name);
    if(iter != sessions_.end())
    {
        return iter->second;
    }

    // 2. 没查找到创建新的
    // 首先必须用配置文件才能创建，找到domain和app，使用配置创建
    auto list = StringUtils::SplitString(session_name, "/");
    if(list.size() != 3) // dommain/app/stream_name
    {
        LIVE_ERROR << "create session failed. Invalid session name:" << session_name;
        return session_null;
    }

    ConfigPtr config = sConfigMgr->GetConfig();
    // list[0] : domain_name  , list[1] : appname 
    auto app_info = config->GetAppInfo(list[0], list[1]);
    if(!app_info) // 没找到配置
    {
        LIVE_ERROR << "create session failed. can't found config. domain:" << list[0]
                    << ",app:"<< list[1];
        return session_null;
    }

    // 找到配置就创建
    auto s = std::make_shared<Session>(session_name);
    s->SetAppInfo(app_info);

    // 添加一个session
    sessions_.emplace(session_name, s);
    LIVE_DEBUG << "create session success. session_name:" << session_name << " now:" << base::TTime::NowMS();

    return s;
}

SessionPtr LiveService::FindSession(const std::string &session_name)
{
    std::lock_guard<std::mutex> lk(lock_);
    // 1. 查找到直接返回
    auto iter = sessions_.find(session_name);
    if(iter != sessions_.end())
    {
        return iter->second;
    }

    return session_null;
}

/// @brief  关闭指定的session
/// @param session_name 
/// @return 
bool LiveService::CloseSession(const std::string &session_name)
{
    SessionPtr s;
    std::lock_guard<std::mutex> lk(lock_);
    // 1. 查找到直接返回
    auto iter = sessions_.find(session_name);
    if(iter != sessions_.end())
    {
        s = iter->second;
        sessions_.erase(iter);
    }
    else
    {
        return false;
    }

    if(s)
    {
        // session管理的publisher和player都关闭
        LIVE_INFO << "close session : " << s->SessionName() 
                << " now:" << base::TTime::NowMS();
        s->Clear();
    }

    return true;
}

/// @brief 流或者session超时检测
/// @param t 
void LiveService::OnTimer(const TaskPtr &t)
{
    std::lock_guard<std::mutex> lk(lock_);
    for(auto iter = sessions_.begin(); iter != sessions_.end();)
    {
        // 流超时或者，长时间没有用户超时
        // 超时就，清理session和删除
        if(iter->second->IsTimeout())
        {
            LIVE_INFO << "session : " << iter->second->SessionName() 
                    << " is timeout. close it. now:" << base::TTime::NowMS();
            iter->second->Clear();
            iter = sessions_.erase(iter);
        }
        else
        {
            ++iter;
        }
    }
    t->Restart(); // 不断重启任务检测超时
}

/// @brief 这是传输层的连接，rtmp需要用这个做点事，但是业务层用不到，纯虚函数必须实现一下
/// @param conn 
void LiveService::OnNewConnection(const TcpConnectionPtr &conn)
{

}

void LiveService::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    // 这个user上下文是在session管理用户CreatePublishUser和playerCreate加入到conn中的
    auto user = conn->GetContext<User>(kUserContext);
    if(user)
    {
        // 关闭用户
        user->GetSession()->CloseUser(user);
    }
}

/// @brief 在收到播放请求之后，添加用户进行tcpConnection的Actie，然后不断激活到顶层，也就是这里，然后进行数据的发送
/// @param conn 
void LiveService::OnActive(const ConnectionPtr &conn)
{
    auto user = conn->GetContext<PlayerUser>(kUserContext);
    // 判断player用户
    if(user && user->GetUserType() >= UserType::kUserTypePlayerPav)
    {
        // 激活之后发送数据
        user->PostFrames();
    }
    // else // 没有用户
    // {
    //     LIVE_ERROR << "no user found. host:" << conn->PeerAddr().ToIpPort();
    //     conn->ForceClose();
    // }
}

/// @brief rtmp播放调用，创建player用户 
/// @param conn 
/// @param session_name 
/// @param param 
/// @return 
bool LiveService::OnPlay(const TcpConnectionPtr &conn, 
                        const std::string &session_name, 
                        const std::string &param)
{
    LIVE_DEBUG << "on play session name:" << session_name 
                << " param:" << param
                << " host:" << conn->PeerAddr().ToIpPort()
                << " now :" << TTime::NowMS();
    
    // 创建session管理
    auto s = CreateSession(session_name);
    if(!s)
    {
        LIVE_ERROR << "create session failed. session name:" << session_name;
        conn->ForceClose();
        return false;
    }

    // 创建用户
    auto user = s->CreatePlayerUser(conn, session_name, param, UserType::kUserTypePlayerRtmp);
    if(!user)
    {
        LIVE_ERROR << "create playerUser failed. session name:" << session_name;
        conn->ForceClose();
        return false;
    }

    conn->SetContext(kUserContext, user);
    // 往session里面添加用户
    s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));
    return true;
}

/// @brief rtmp创建publisher
/// @param conn 
/// @param session_name 
/// @param param 
/// @return 
bool LiveService::OnPublish(const TcpConnectionPtr &conn, 
                        const std::string &session_name, 
                        const std::string &param)
{
       LIVE_DEBUG << "on publish session name:" << session_name 
                << " param:" << param
                << " host:" << conn->PeerAddr().ToIpPort()
                << " now :" << TTime::NowMS();
    
    // 创建session管理
    auto s = CreateSession(session_name);
    if(!s)
    {
        LIVE_ERROR << "create session failed. session name:" << session_name;
        conn->ForceClose();
        return false;
    }

    // 创建publisher
    auto user = s->CreatePublishUser(conn, session_name, param, UserType::kUserTypePublishRtmp);
    if(!user)
    {
        LIVE_ERROR << "create PublishUser failed. session name:" << session_name;
        conn->ForceClose();
        return false;
    }

    conn->SetContext(kUserContext, user);
    s->SetPublisher(user);
    return true;
}

/// @brief rtmp收到流数据回调
/// @param conn 
/// @param data 
void LiveService::OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data)
{
    // 推流用户
    auto user = conn->GetContext<User>(kUserContext);
    if(!user)
    {
        LIVE_ERROR << "no found user. host:" << conn->PeerAddr().ToIpPort() ;
        conn->ForceClose();
        return;
    }

    // 把收到的数据加入到实时流中，就是推流上来的数据data，加入到播放的buffer中
    user->GetStream()->AddPacket(std::move(data));
}

/// @brief 回调用过httpContext进行调用
/// @param conn 
void LiveService::OnSent(const TcpConnectionPtr &conn)
{
}

bool LiveService::OnSentNextChunk(const TcpConnectionPtr &conn)
{
    
    return false;
}

/// @brief 收到一个http请求,可以发送flv，这是httpcontext的上层业务回调函数
/// @param conn 
/// @param req 
/// @param packet 
void LiveService::OnRequest(const TcpConnectionPtr &conn, const HttpRequestPtr &req, const PacketPtr &packet)
{
    if(req->IsRequest())
    {
        LIVE_DEBUG << "req method:" << req->Method() << " path:" << req->Path();
    }
    else
    {
        LIVE_DEBUG << "req code:" << req->GetStatusCode() << " msg:" << HttpUtils::ParseStatusMessage(req->GetStatusCode());
    }

    auto headers = req->Headers();
    for(auto const &h : headers)
    {
        LIVE_DEBUG << h.first << ":" << h.second;
    }

    // 收到请求
    if(req->IsRequest())
    {
        // http://ip:port/domain/app/stream.flv
        // http://ip:potr/domain/app/stream/filename.flv

        auto list = base::StringUtils::SplitString(req->Path(), "/");
        if(list.size() < 4) // 至少4个，不然请求非法
        {
            auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
            if(http_cxt)
            {
                auto res = HttpRequest::NewHttp400Response();
                http_cxt->PostRequest(res);
                return;
            }
        }

        const std::string &domain = list[1];
        const std::string &app = list[2];
        string filename = list[3];
        std::string stream_name;
        if(list.size() > 4) // http://ip:potr/domain/app/stream/filename.flv
        {
            filename = list[4];
            stream_name = list[3];
        }
        else  // http://ip:port/domain/app/filename.flv
        {
            stream_name = base::StringUtils::FileName(filename); // 去.flv
        }

        std::string ext = base::StringUtils::Extension(filename); // 后缀
        if(ext == "flv")
        {
            // 要传输flv了，创建一个session管理用户stream
            std::string session_name = domain + "/" + app + "/" + stream_name;
            LIVE_DEBUG << "flv play session name:" << session_name;
            // 查找session_name。有没有这个推流存在，如果有就用flv封装用http发送播放
            // 没有就创建一个新的（配置文件里面存在的）
            auto s = CreateSession(session_name);
            if(!s)
            {
                LIVE_ERROR << "can't create session name:" << session_name; 
                auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
                if(http_cxt)
                {
                    // 如果服务器出问题主动关闭连接，就会出现大量的time_wait状态
                    // 返会404，客户端就会主动关闭
                    auto res = HttpRequest::NewHttp404Response();
                    http_cxt->PostRequest(res);
                    return;
                }
            }

            auto user = s->CreatePlayerUser(conn, session_name, "", UserType::kUserTypePlayerFlv);
            if(!user)
            {
                LIVE_ERROR << "can't create user session name:" << session_name; 
                auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
                if(http_cxt)
                {
                    auto res = HttpRequest::NewHttp404Response();
                    http_cxt->PostRequest(res);
                    return;
                }
            }
            conn->SetContext(kUserContext, user);
            // 因为flv使用的是httpcontext处理发送，所以调用不到flvContext，这里创建一个
            auto flv = std::make_shared<FlvContext>(conn, this);
            // 设置到tcpConnection，和http用的同一个conn,httpserver也能拿到这个上下文
            conn->SetContext(kFlvContext, flv);
            // 用户加入到播放session的播放列表
            s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));
        }
    }
}

void LiveService::Start()
{
    // 1. 启动循环线程池
    ConfigPtr config = sConfigMgr->GetConfig();
    pool_ = new EventLoopThreadPool(config->thread_num_, config->cpu_start_, config->cpus_);
    pool_->Start();

    sDnsService->Start(); //     启动dns服务,不断解析本地地址和域名
    // 2. 启动服务
    auto services = config->GetServiceInfos();
    auto eventloops = pool_->GetLoops(); // 启动loop并且获取
    LIVE_DEBUG << "eventloop size = " << eventloops.size();
    for(auto & el : eventloops)
    {
        for(auto &s : services)
        {
            LIVE_DEBUG << "s->protocpl:" << s->protocol << "  s->port:" << s->port;
            if(s->protocol == "RTMP" || s->protocol == "rtmp")
            {
                InetAddress local(s->addr, s->port);
                // rtmpserver继承tcpserver，this传入作为业务上层，有数据都会通知下面的回调
                TcpServer* server = new RtmpServer(el, local, this);
                servers_.push_back(server);
                // 启动tcpServer，设置accept
                servers_.back()->Start();
            }
            else if(s->protocol == "HTTP" || s->protocol == "http")
            {
                InetAddress local(s->addr, s->port);
                // rtmpserver继承tcpserver，this传入作为业务上层，有数据都会通知下面的回调
                TcpServer* server = new HttpServer(el, local, this);
                servers_.push_back(server);
                // 启动tcpServer，设置accept
                servers_.back()->Start();
            }
        }
    }

    // 3. 设置定时检测超时任务，5s一次，越早检测出越好,越节省带宽
    TaskPtr t = std::make_shared<Task>(std::bind(&LiveService::OnTimer, this, std::placeholders::_1), 5000);
    sTaskMgr->Add(t);
}

/// @brief 可以不实现，业务服务一跑起来就不停了，如果要用就用SININT等，系统会回收资源
void LiveService::Stop()
{
}

/// @brief 随机返回一个eventloop（按顺序拿的）
/// @return 
EventLoop *LiveService::GetNextLoop()
{
    return pool_->GetNextLoop();
}
