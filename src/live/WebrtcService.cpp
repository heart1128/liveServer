/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 12:12:45
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:34:43
 * @FilePath: /liveServer/src/live/WebrtcService.cpp
 * @Description:  learn 
 */
#include "WebrtcService.h"
#include "mmedia/http/HttpServer.h"
#include "mmedia/http/HttpRequest.h"
#include "mmedia/http/HttpUtils.h"
#include "mmedia/http/HttpContext.h"
#include "live/base/LiveLog.h"
#include "base/StringUtils.h"
#include "mmedia/webrtc/Stun.h"
#include "live/Session.h"
#include "live/LiveService.h"
#include "live/user/WebrtcPlayUser.h"
#include "mmedia/webrtc/WebrtcServer.h"
#include <json/json.h>

using namespace tmms::live;
using namespace tmms::mm;

/**
 * @description: 收到udp的Onread()->webrtcServer()的MessageCallback()到这
 *                  进行stun的解析和组装回复
 *          
 *          stun如果有客户端请求，服务端必须不断响应，不然会认为断开连接
 * @param {UdpSocketPtr} &socket
 * @param {InetAddress} &addr
 * @param {MsgBuffer} &buf
 * @return {*}
 */
void WebrtcService::OnStun(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
    LIVE_DEBUG << "on stun size: " << buf.ReadableBytes();
    
    Stun stun;
    if(!stun.Decode(buf.Peek(), buf.ReadableBytes()))
    {
        LIVE_ERROR << "stun decode error.";
        return;
    }

    WebrtcPlayUserPtr webrtc_user;
    std::lock_guard<std::mutex> lk(lock_);
    auto iter = name_users_.find(stun.LocalUFrag());
    if(iter != name_users_.end())
    {
        webrtc_user = iter->second;
        
        stun.SetPassword(webrtc_user->LocalPasswd());
        // 因为之前sdp用的是http，使用tcpConnection
        // 所以在header中设置了close，后面都要用udp传输，设置为udpConnection到用户
        webrtc_user->SetConnection(socket); 
        // 后续是udp传输，没有uFrag信息了，所以使用固定的ip port保存用户
        // users_.emplace(addr.ToIpPort(), webrtc_user);
        // 设置地址
        webrtc_user->SetSockAddr(addr);

        // stun如果有客户端请求，服务端必须不断响应，不然会认为断开连接
        // 所以每次都要发送
        stun.SetMessageType(kStunMsgBindingResponse);
        stun.SetMappedAddr(addr.IPv4());
        stun.SetMappedPort(addr.Port());

        PacketPtr packet = stun.Encode();
        if(packet)
        {
            // 拿到socket地址，使用webrtcserver打包发送
            auto server = sLiveService->GetWebrtcServer();
            auto naddr = webrtc_user->GetSockAddr();
            // 每个用户设置一个地址，因为每个用户的ip port都是不一样的
            packet->SetExt(naddr);
            server->SendPacket(packet);
        }
    }

    if(webrtc_user)
    {
        // 用户没有加入要加入
        auto iter1 = users_.find(addr.ToIpPort());
        if(iter1 == users_.end())
        {
            users_.emplace(addr.ToIpPort(), webrtc_user);
        }
    }
}

/**
 * @description: udp收到消息给webserver，判断类型调用这个函数，然后再调用dtls的开始
 * @param {UdpSocketPtr} &socket
 * @param {InetAddress} &addr
 * @param {MsgBuffer} &buf
 * @return {*}
 */
void WebrtcService::OnDtls(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
    // 添加一个用户和连接的映射，因为在之后的数据传输中不是使用的tcp，换成udp了
    auto iter = users_.find(addr.ToIpPort());
    if(iter != users_.end())
    {
        auto webrtc_user = iter->second;
        webrtc_user->OnDtlsRecv(buf.Peek(), buf.ReadableBytes());
        buf.RetrieveAll();
    }
}

void WebrtcService::OnRtp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
     LIVE_DEBUG << "on rtp size: " << buf.ReadableBytes();
}

void WebrtcService::OnRtcp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
    // 找到用户，调用接口
    auto iter = users_.find(addr.ToIpPort());
    if(iter != users_.end())
    {
        auto webrtc_user = iter->second;
        webrtc_user->OnRtcp(buf.Peek(), buf.ReadableBytes());
        buf.RetrieveAll();
    }
}

void WebrtcService::OnRequest(const TcpConnectionPtr &conn, const HttpRequestPtr &req, const PacketPtr &packet)
{
    auto http_cxt = conn->GetContext<HttpContext>(kHttpContext);
    if(!http_cxt)
    {
        LIVE_ERROR << "no found http context.something must be wrong.";
        return;
    }
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
        if(req->Method() == kOptions)
        {
            auto res = HttpRequest::NewHttpOptionsResponse();
            http_cxt->PostRequest(res);
            return;
        }
        LIVE_DEBUG << "req->Path(): " << req->Path();
        // 每个请求都有不同的路径，需要判断
        if(req->Path() == "/rtc/v1/play/" && packet)
        {
            LIVE_DEBUG << "request: \n" << packet->Data();
            // 传入的请求是json,包含dsp
            Json::CharReaderBuilder builder;
            Json::Value root;
            Json::String err;
            std::unique_ptr<Json::CharReader> reader(builder.newCharReader());
            if(!reader->parse(packet->Data(), packet->Data() + packet->PacketSize(), &root, &err))
            {
                LIVE_ERROR << "parse json error.";
                auto res = HttpRequest::NewHttp400Response();
                http_cxt->PostRequest(res);
                return;
            }
            
            auto api = root["api"].asString();
            auto streamUrl = root["streamurl"].asString();
            auto clientIp = root["clientip"].asString();
            auto sdp = root["sdp"].asString();
            
            std::string session_name = GetSessionNameFromUrl(streamUrl);
            LIVE_DEBUG << "get session name: " << session_name;

            // 创建一个session
            // 所有的session还是liveService管理
            auto s = sLiveService->CreateSession(session_name);
            if(!s)
            {
                LIVE_ERROR << "cant create session name: " << session_name;
                auto res = HttpRequest::NewHttp400Response();
                http_cxt->PostRequest(res);
                return;
            }

            // 创建用户
            auto user = s->CreatePlayerUser(conn, session_name, "", UserType::kUserTypePlayerWebRTC);
            if(!user)
            {
                LIVE_ERROR << "cant create user session name: " << session_name;
                auto res = HttpRequest::NewHttp400Response();
                http_cxt->PostRequest(res); 
                return;
            }

            // 用户加入到session
            // 每个session管理多个用户，每个用户也在使用session
            s->AddPlayer(std::dynamic_pointer_cast<PlayerUser>(user));

            // 开始解析sdp
            auto webrtc_user = std::dynamic_pointer_cast<WebrtcPlayUser>(user);
            if(!webrtc_user->ProcessOfferSdp(sdp))
            {
                LIVE_ERROR << "parse sdp error. session_name: " << session_name;
                auto res = HttpRequest::NewHttp400Response();
                http_cxt->PostRequest(res); 
                return;
            }

            // 解析之后sdp需要返回一个回答（回答是带证书指纹的）
            auto answer_sdp = webrtc_user->BuildAnswerSdp();
            LIVE_DEBUG << "answer sdp: \n" << answer_sdp;

            Json::Value result;
            result["code"] = 0;
            result["server"] = "tmms";
            result["sdp"] = std::move(answer_sdp);
            result["sessionid"] = webrtc_user->RemoteUFrag() + ":" + webrtc_user->LocalUFrag();

            auto content = result.toStyledString();

            auto res = std::make_shared<HttpRequest>(false);
            res->SetStatusCode(200);
            res->AddHeader("server","tmms");
            res->AddHeader("content-length",std::to_string(content.size()));
            res->AddHeader("content-type","text/plain");    
            res->AddHeader("Access-Control-Allow-Origin","*");
            res->AddHeader("Access-Control-Allow-Methods","POST,GET,OPTIONS");
            res->AddHeader("Allow","POST,GET,OPTIONS");
            res->AddHeader("Access-Control-Allow-Headers","content-type");
            res->AddHeader("Connection", "close"); // 发送完就关闭，不是保持链接的
            res->SetBody(content);
            http_cxt->PostRequest(res);

            // 保存用户名对应的用户
            std::lock_guard<std::mutex> lk(lock_);
            name_users_.emplace(webrtc_user->LocalUFrag(), webrtc_user);
        }
    }
}


/**
 * @description: 在rtmp中推流上来的stream有数据就会调用session中激活所有用户
 *      继而调用这个函数进行发送，因为webrtc是点对点的，不需要进行拉推
 * @return {*}
 */
void WebrtcService::Push2Players()
{
    // 给webrtc发送用户所有数据
    std::lock_guard<std::mutex> lk(user_lock_);
    for(auto &user : users_)
    {
        user.second->PostFrames();
    }
}


/**
 * @description: 分割url，返回session_name
 * @param {string} &url
 * @return {*}
 */
std::string WebrtcService::GetSessionNameFromUrl(const std::string &url)
{
    // webrtc://heart.com:8081/live/test
    // webrtc://heart.com:8081/domain/live/test

    auto list = base::StringUtils::SplitString(url, "/");

    if(list.size() < 5)
    {
        return "";
    }

    std::string domain, app, stream;
    if(list.size() == 5)
    {
        domain = list[2];
        app = list[3];
        stream = list[4];
    }
    else if(list.size() == 6)
    {
        domain = list[3];
        app = list[4];
        stream = list[5];
    }

    auto pos = domain.find_first_of(":");
    if(pos != std::string::npos)
    {
        domain = domain.substr(0, pos);
    }

    return domain + "/" + app + "/" + stream;
}
