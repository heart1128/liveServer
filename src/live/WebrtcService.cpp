/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 12:12:45
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 15:35:44
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
#include "live/Session.h"
#include "live/LiveService.h"
#include "live/user/WebrtcPlayUser.h"
#include <json/json.h>

using namespace tmms::live;
using namespace tmms::mm;

void WebrtcService::OnStun(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
    LIVE_DEBUG << "on stun size: " << buf.ReadableBytes();
}

void WebrtcService::OnDtls(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
}

void WebrtcService::OnRtp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
}

void WebrtcService::OnRtcp(const network::UdpSocketPtr &socket, const network::InetAddress &addr, network::MsgBuffer &buf)
{
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
        }
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
