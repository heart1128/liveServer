/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 15:17:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:55:19
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
    
    udp_server_->SetWriteCompleteCallback(std::bind(&WebrtcServer::WriteComplete, this,
                                            std::placeholders::_1));
    udp_server_->Start();
    WEBRTC_DEBUG << "webrtc server start.";
}

void WebrtcServer::SendPacket(const PacketPtr & packet)
{
    std::lock_guard<std::mutex> lk(lock_);
    // 有数据正在发送，等待，加入等待队列
    if(b_sending_)
    {
        out_waiting_.emplace_back(packet);
        return;
    }
    
    // 没有数据在发送，加入发送队列
    udp_outs_.clear();
    std::shared_ptr<struct sockaddr_in6> addr = packet->Ext<struct sockaddr_in6>();
    if(addr)
    {
        UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(packet->Data(), packet->PacketSize()
                                            , (struct sockaddr*)addr.get(), sizeof(struct sockaddr_in6));
        udp_outs_.emplace_back(std::move(up)); // 发送过的
        sending_.emplace_back(packet); // 准备发送的
    }

    // 使用udp进行发送数据
    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    }
}

void WebrtcServer::SendPacket(std::list<PacketPtr> &list)
{
    std::lock_guard<std::mutex> lk(lock_);

    // 有数据正在发送，等待，加入等待队列
    if(b_sending_)
    {
        for(auto &p : list)
        {
            out_waiting_.emplace_back(p);
        }
        return;
    }
    
    // 没有数据在发送，加入发送队列
    udp_outs_.clear();
    for(auto &p : list)
    {
        std::shared_ptr<struct sockaddr_in6> addr = p->Ext<struct sockaddr_in6>();
        if(addr)
        {
            UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(p->Data(), p->PacketSize()
                                                , (struct sockaddr*)addr.get(), sizeof(struct sockaddr_in6));
            udp_outs_.emplace_back(std::move(up)); // 发送过的
            sending_.emplace_back(p); // 准备发送的
        }
    }


    // 使用udp进行发送数据
    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    }
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
    return buf.ReadableBytes() >= 20 && data[0] >= 0 && data[0] <= 3;
}

/// @brief 长度至少为20字节 ，第一个字节为[128,191], 第二个字节不在[192, 223]
/// @param buf 
/// @return 
bool WebrtcServer::IsRtp(MsgBuffer &buf)
{
    const char *data = buf.Peek();
    uint8_t pt = (uint8_t)data[1];
    return buf.ReadableBytes() >= 12 && (data[0] & 0x80) && !(pt >= 192 && pt <= 223);
}

/// @brief 长度至少为20字节 ，第一个字节为[128,191], 第二个字节在[192, 223]
/// @param buf 
/// @return 
bool WebrtcServer::IsRtcp(MsgBuffer &buf)
{
    const char *data = buf.Peek();
     uint8_t pt = (uint8_t)data[1];
    return buf.ReadableBytes() >= 12 && (data[0] & 0x80) && (pt >= 192 && pt <= 223);
}

void WebrtcServer::OnSend()
{
    std::lock_guard<std::mutex> lk(lock_);
    // 在发送中或者需要发送的为空
    if(b_sending_ || out_waiting_.empty())
    {
        return;
    }
    udp_outs_.clear();
    for(auto &p : out_waiting_)
    {
        std::shared_ptr<struct sockaddr_in6> addr = p->Ext<struct sockaddr_in6>();
        if(addr)
        {
            UdpBufferNodePtr up = std::make_shared<UdpBufferNode>(p->Data(), p->PacketSize()
                                                , (struct sockaddr*)addr.get(), sizeof(struct sockaddr_in6));
            udp_outs_.emplace_back(std::move(up)); // 发送过的
            sending_.emplace_back(p); // 准备发送的
        }
    }
    out_waiting_.clear();
    // 使用udp进行发送数据
    if(!udp_outs_.empty())
    {
        b_sending_ = true;
        auto socket = std::dynamic_pointer_cast<UdpSocket>(udp_server_);
        socket->Send(udp_outs_);
    }
}

/**
 * @description: 写完一次udp，返回接着发送
 * @param {UdpSocketPtr} &socket
 * @return {*}
 */
void WebrtcServer::WriteComplete(const UdpSocketPtr &socket)
{
    std::lock_guard<std::mutex> lk(lock_);
    b_sending_ = false;
    sending_.clear();
    udp_outs_.clear();

    // 有数据就一直发送
    if(!out_waiting_.empty())
    {
        OnSend();
    }
}
