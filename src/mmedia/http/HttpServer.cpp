/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-10 14:43:04
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-04 16:05:25
 */
#include "HttpServer.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/http/HttpContext.h"
#include "mmedia/flv/FlvContext.h"

using namespace tmms::mm;

HttpServer::HttpServer(EventLoop *loop, const InetAddress &local, HttpHandler *handler)
:TcpServer(loop, local), http_handler_(handler)
{
}

HttpServer::~HttpServer()
{
}

/// @brief 全部由tcpserver处理回调，但是调用的都是HttpServer设置的函数
void HttpServer::Start()
{
    TcpServer::SetActiveCallback(std::bind(&HttpServer::OnActive,this, std::placeholders::_1));
    TcpServer::SetDestroyConnectionCallback(std::bind(&HttpServer::OnConnectionDestroy,this, std::placeholders::_1));
    TcpServer::SetNewConnectionCallback(std::bind(&HttpServer::OnNewConnection,this, std::placeholders::_1));
    TcpServer::SetWriteCompleteCallback(std::bind(&HttpServer::OnWriteComplete,this, std::placeholders::_1));
    TcpServer::SetMessageCallback(std::bind(&HttpServer::OnMessage,this, std::placeholders::_1, std::placeholders::_2));
    TcpServer::Start();
    HTTP_DEBUG << "Http server start.";
}

void HttpServer::Stop()
{
    TcpServer::Stop();
}

/// @brief  新连接回调
/// @param conn 
void HttpServer::OnNewConnection(const TcpConnectionPtr &conn)
{
    // 1. 通知上层
    if(http_handler_)
    {
        http_handler_->OnNewConnection(conn);
    }
    // 2. 处理http服务端连接
    HttpContextPtr shake = std::make_shared<HttpContext>(conn, http_handler_);
    conn->SetContext(kHttpContext, shake);
}

void HttpServer::OnConnectionDestroy(const TcpConnectionPtr &conn)
{
    // 1. 通知上层
    if(http_handler_)
    {
        http_handler_->OnConnectionDestroy(conn);
    }
    conn->ClearContext(kHttpContext);
}

/// @brief 接收到消息，就要开始改变握手的状态
/// @param conn 
/// @param buf 
void HttpServer::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    HttpContextPtr shake = conn->GetContext<HttpContext>(kHttpContext);
    if(shake)
    {   
        
        int ret = shake->Parse(buf); 
        if(ret == -1)  // -1失败
        {
            conn->ForceClose();
        }
    }
}

/// @brief 写完成之后，就要改变握手状态机的状态
/// @param conn 
void HttpServer::OnWriteComplete(const ConnectionPtr &conn)
{
    HttpContextPtr shake = conn->GetContext<HttpContext>(kHttpContext);
    if(shake)
    {
        shake->WriteComplete(std::dynamic_pointer_cast<TcpConnection>(conn)); // 运转状态机
    }
    // 设置的dlvconetext是在处理请求设置的，肯定比服务端发送完成的时间要早
    // 因为是tcpconnetion传入的回调，贯穿传输到业务，在liveServer设置的flvConetext也能拿到
    FlvContextPtr flv = conn->GetContext<FlvContext>(kFlvContext); 
    if(flv)
    {
        flv->WriterComplete(std::dynamic_pointer_cast<TcpConnection>(conn));
    }
}

void HttpServer::OnActive(const ConnectionPtr &conn)
{
    if(http_handler_)
    {
        http_handler_->OnActive(conn);
    }
}
