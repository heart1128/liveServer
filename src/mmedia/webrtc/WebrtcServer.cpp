/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 15:17:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 16:36:09
 * @FilePath: /liveServer/src/mmedia/webrtc/WebrtcServer.cpp
 * @Description:  learn 
 */
#include "WebrtcServer.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

WebrtcServer::WebrtcServer(EventLoop *loop, const InetAddress &server, WebrtcHandler *handler)
: handler_(handler)
{
    udp_server_ = std::make_shared<UdpServer>(loop, server);
}

void WebrtcServer::Start()
{
    // 借用udp的服务端监听收到消息
    udp_server_->SetRecvMsgCallback(std::bind(&WebrtcServer::MessageCallback, this,
                                            std::placeholders::_1,
                                            std::placeholders::_2,
                                            std::placeholders::_3));
    udp_server_->Start();
    WEBRTC_DEBUG << "webrtc server start.";
}

void WebrtcServer::MessageCallback(const UdpSocketPtr &socket, const InetAddress &addr, MsgBuffer &buf)
{
    // 交给webSerivce处理
    if(handler_)
    {
        // NAT打洞
        if(IsStun(buf))
        {
            handler_->OnStun(socket, addr, buf);
        }
        else if(IsDtls(buf))
        {
            handler_->OnDtls(socket, addr, buf);
        }
        else if(IsRtp(buf)) 
        {
            handler_->OnRtp(socket, addr, buf);
        }
        else if(IsRtcp(buf))
        {
            handler_->OnRtcp(socket, addr, buf);
        }
    }
}

/// @brief  长度至少为13字节，第一个字节为[20,63]
/// @param buf 
/// @return 
bool WebrtcServer::IsDtls(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    return buf.ReadableBytes() >= 13 && data[0] >= 20 && data[0] <= 63;
}

/// @brief 长度至少为12字节 ，第一个字节为[0,3]
/// @param buf 
/// @return 
bool WebrtcServer::IsStun(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    return buf.ReadableBytes() >= 12 && data[0] >= 0 && data[0] <= 3;
}

/// @brief 长度至少为20字节 ，第一个字节为[128,191], 第二个字节不在[192, 223]
/// @param buf 
/// @return 
bool WebrtcServer::IsRtp(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    return buf.ReadableBytes() >= 13 && 
        (data[0] >= 0 && data[0] <= 3) &&
        !(data[1] >= 192 && data[1] <= 223);
}

/// @brief 长度至少为20字节 ，第一个字节为[128,191], 第二个字节在[192, 223]
/// @param buf 
/// @return 
bool WebrtcServer::IsRtcp(MsgBuffer &buf)
{
        const char *data = buf.Peek();
    return buf.ReadableBytes() >= 13 && 
        (data[0] >= 0 && data[0] <= 3) &&
        (data[1] >= 192 && data[1] <= 223);
}
