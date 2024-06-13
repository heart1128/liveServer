/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-10 14:43:04
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-13 10:14:16
 */
#include "RtmpServer.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/rtmp/RtmpContext.h"

using namespace tmms::mm;

RtmpServer::RtmpServer(EventLoop *loop, const InetAddress &local, RtmpHandler *handler)
:TcpServer(loop, local), rtmp_handler_(handler)
{
}

RtmpServer::~RtmpServer()
{
}

/// @brief 全部由tcpserver处理回调，但是调用的都是rtmpserver设置的函数
void RtmpServer::Start()
{
    TcpServer::SetActiveCallback(std::bind(&RtmpServer::OnActive,this, std::placeholders::_1));
    TcpServer::SetDestroyConnectionCallback(std::bind(&RtmpServer::OnConnectionDestroy,this, std::placeholders::_1));
    TcpServer::SetNewConnectionCallback(std::bind(&RtmpServer::OnNewConnection,this, std::placeholders::_1));
    TcpServer::SetWriteCompleteCallback(std::bind(&RtmpServer::OnWriteComplete,this, std::placeholders::_1));
    TcpServer::SetMessageCallback(std::bind(&RtmpServer::OnMessage,this, std::placeholders::_1, std::placeholders::_2));
    TcpServer::Start();
}

void RtmpServer::Stop()
{
    TcpServer::Stop();
}

/// @brief  新连接回调
/// @param conn 
void RtmpServer::OnNewConnection(const TcpConnectionPtr &conn)
{
    // 1. 通知上层
    if(rtmp_handler_)
    {
        rtmp_handler_->OnNewConnection(conn);
    }
    // 2. 处理rtmp服务端连接
    RtmpContextPtr shake = std::make_shared<RtmpContext>(conn, nullptr);
    conn->SetContext(kRtmpContext, shake);
    shake->StarHandShake(); // 等待C0C1
}

void RtmpServer::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    // 1. 通知上层
    if(rtmp_handler_)
    {
        rtmp_handler_->OnConnectionDestroy(conn);
    }
    conn->ClearContext(kRtmpContext);
}

/// @brief 接收到消息，就要开始改变握手的状态
/// @param conn 
/// @param buf 
void RtmpServer::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    RtmpContextPtr shake = conn->GetContext<RtmpContext>(kRtmpContext);
    // RtmpHandShakePtr shake = conn->GetContext<RtmpHandShake>(kRtmpContext);
    if(shake)
    {   
        // int ret = shakehand->HandShake(buf); // 开始握手
        int ret = shake->Parse(buf); // 开始握手
        if(ret == 0)        // 0握手成功
        {
            RTMP_DEBUG << "host: " << conn->PeerAddr().ToIpPort() << " handshake success.";
        }
        else if(ret == -1)  // -1失败
        {
            conn->ForceClose();
        }
    }
}

/// @brief 写完成之后，就要改变握手状态机的状态
/// @param conn 
void RtmpServer::OnWriteComplete(const ConnectionPtr &conn)
{
    RtmpContextPtr shake = conn->GetContext<RtmpContext>(kRtmpContext);
    if(shake)
    {
        shake->OnWriteComplete(); // 运转状态机
    }
}

void RtmpServer::OnActive(const ConnectionPtr &conn)
{
    if(rtmp_handler_)
    {
        rtmp_handler_->OnActive(conn);
    }
}
