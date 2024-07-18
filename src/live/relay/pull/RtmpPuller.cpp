/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-18 14:09:20
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-18 14:45:30
 * @FilePath: /liveServer/src/live/relay/pull/RtmpPuller.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#include "RtmpPuller.h"
#include "live/base/LiveLog.h"
#include "live/Stream.h"

using namespace tmms::live;

RtmpPuller::RtmpPuller(EventLoop *loop, Session *s, PullHandler *handler)
:Puller(loop, s, handler)
{
}

RtmpPuller::~RtmpPuller()
{
    if(rtmp_client_)
    {
        delete rtmp_client_;
        rtmp_client_ = nullptr;
    }
}

/// @brief 回源，使用rtmpCient获取回源，然后这个user作为publisher
/// @param target 
/// @return 
bool RtmpPuller::Pull(const TargetPtr &target)
{
    // rtmp回源
    if(target->protocol == "RTMP" || target->protocol == "rtmp")
    {
        if(rtmp_client_)
        {
            delete rtmp_client_;
            rtmp_client_ = nullptr;
        }
        if(!rtmp_client_)
        {
            // 自身继承rtmpHandler作为rtmpClient的回调上层
            rtmp_client_ = new RtmpClient(event_loop_, this);
                // 设置关闭
            rtmp_client_->SetCloseCallback([this](const TcpConnectionPtr &conn){
                if(pull_handler_)
                {
                    // TODO：应该设置参数，知道是主动断开还是异常断开，有些断开不需要重连
                    pull_handler_->OnPullClose();
                }
            });
        }


        // 使用target组装rtmp地址，使用rtmpClient拉流
        std::stringstream ss;
        
        ss << "rtmp://" << target->remote_host
            << ":" << target->remote_port
            << "/" << target->domain_name
            << "/" << target->app_name
            << "/" << target->stream_name;
        
        PULLER_DEBUG << "rtmp client pull:" << ss.str();
        
        targer_ = target;

        rtmp_client_->Play(ss.str());
        return true;
    }
    return false;
}

void RtmpPuller::OnNewConnection(const TcpConnectionPtr &conn)
{
    
}

void RtmpPuller::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    // 关闭的时候，清除用户
    UserPtr user = conn->GetContext<User>(kUserContext);
    if(user)
    {
        if(session_)
        {
            session_->CloseUser(user);
        }
    }
}

void RtmpPuller::OnRecv(const TcpConnectionPtr &conn, PacketPtr &&data)
{
    UserPtr user = conn->GetContext<User>(kUserContext);
    if(!user)
    {
        PULLER_ERROR << "no found user.host:" << conn->PeerAddr().ToIpPort();
        return;
    }
    // 收到的数据包给stream,进行分析
    session_->GetStream()->AddPacket(std::move(data));
}

/// @brief 这个player是相对回源服务器的，拉流下来之后推流到需要的地方
/// @param conn 
/// @param session_name 
/// @param param 
/// @return 
bool RtmpPuller::OnPlay(const TcpConnectionPtr &conn, const std::string &session_name, const std::string &param)
{
    UserPtr user = conn->GetContext<User>(kUserContext);
    if(!user)
    {
        // 没有用户，创建
        user = session_->CreatePublishUser(std::dynamic_pointer_cast<Connection>(conn),
                                targer_->session_name, "", UserType::kUserTypePlayerRtmp);
        
        if(!user)
        {
            PULLER_ERROR << "create user failed";
            if(pull_handler_)
            {
                pull_handler_->OnPullClose();
            }
            return false;
        }
        conn->SetContext(kUserContext, user);
    }
    if(pull_handler_)
    {
        pull_handler_->OnPullSucess();
    }
    // 因为当前用户是从回源服务器进行拉流，拉流的时候数据Player
    // 然后需要把回源拉下来的流推流到需要的服务点，所以设置成Publisher
    session_->SetPublisher(user);
    return true;
}
