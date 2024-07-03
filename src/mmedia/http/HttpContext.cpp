#include "HttpContext.h"

using namespace tmms::mm;

namespace
{
    static std::string CHUNK_EOF = "0\r\n\r\n"; // chunk以len包的长度为0结束，0单独在一行
}

HttpContext::HttpContext(const TcpConnectionPtr & conn, HttpHandler * handler)
: connection_(conn), handler_(handler)
{
}

/// @brief 解析http请求的包
/// @param buf 
/// @return 
int32_t HttpContext::Parse(MsgBuffer &buf)
{
    while (buf.ReadableBytes() > 1)
    {
        // 状态机解析
        auto state = http_parser_.Parse(buf);
        if(state == kExpectHttpComplete || state == kExpectChunkComplete)
        {
            if(handler_)
            {
                handler_->OnRequest(connection_, http_parser_.GetHttpRequest(), http_parser_.Chunk());
            }
        }
        else if(state == kExpectError)
        {
            connection_->ForceClose();
        }
    }
    return 1;
}

/// @brief http头和body一起发
/// @param header_and_body 
/// @return 
bool HttpContext::PostRequest(const std::string &header_and_body)
{
    // 只有在初始化的状态才能发，其他状态都是有数据包在发的
    if(post_state_ != kHttpContextPostInit)
    {
        return false;
    }

    header_ = header_and_body;
    post_state_ = kHttpContextPostHttp;
    connection_->Send(header_.c_str(), header_.size());
    return true;
}

/// @brief 先发头，然后流转到body
/// @param header 
/// @param packet 
/// @return 
bool HttpContext::PostRequest(const std::string &header, PacketPtr &packet)
{
    if(post_state_ != kHttpContextPostInit)
    {
        return false;
    }

    header_ = header;
    out_packet_ = packet;
    post_state_ = kHttpContextPostHttpHeader;
    connection_->Send(header_.c_str(), header_.size());
    return true;

}

/// @brief 整体的发送过程，状态机转动
/// @param request 
/// @return 
bool HttpContext::PostRequest(HttpRequestPtr &request)
{
    if(request->IsChunked()) // 是chunk包，就直接发个头就不管了，等待发送len
    {
        PostChunkHeader(request->MakeHeaders());
    }
    else if(request->IsStream())
    {
        PostStreamHeader(request->MakeHeaders());
    }
    else
    {
        // 发送普通的http包，完整的
        PostRequest(request->AppendToBuffer());
    }
    return true;
}

/// @brief 发送chunk header
/// @param header 
/// @return 
bool HttpContext::PostChunkHeader(const std::string &header)
{
    if(post_state_ != kHttpContextPostInit)
    {
        return false;
    }

    header_ = header;
    post_state_ = kHttpContextPostInit;  // 发送头之后什么都不做，因为后面不一定有chunk包来
    connection_->Send(header_.c_str(), header_.size());
    header_sent_ = true;
    return true;
}

void HttpContext::PostChunk(PacketPtr &chunk)
{
    out_packet_ = chunk;
    if(!header_sent_) // 还没有发送chunk header
    {
        post_state_ = kHttpContextPostChunkHeader; 
        connection_->Send(header_.c_str(), header_.size());
        header_sent_ = true;
    }
    else
    {
        post_state_ = kHttpContextPostChunkLen; // 发送了头就发送len
        char buf[32] = {0,};
        sprintf(buf, "%X\r\n", out_packet_->PacketSize());
        header_ = std::string(buf);
        connection_->Send(header_.c_str(), header_.size());
    }
}

/// @brief 发送长度为0的 0\r\n\r\n
void HttpContext::PostEofChunk()
{
    post_state_ = kHttpContextPostChunkEOF; 
    connection_->Send(CHUNK_EOF.c_str(), CHUNK_EOF.size());
}

bool HttpContext::PostStreamHeader(const std::string &header)
{
    if(post_state_ != kHttpContextPostInit)
    {
        return false;
    }

    header_ = header;
    post_state_ = kHttpContextPostInit; // 发了头不一定有数据，等有数据主动发送
    connection_->Send(header_.c_str(), header_.size());
    header_sent_ = true;
    return true;
}

/// @brief 发送流数据，如flv的
/// @param packet 
/// @return 只有上一个发送完了才能发送下一个，不能连续重叠发送，返回一个是否发送完了
bool HttpContext::PostStreamChunk(PacketPtr &packet)
{
    if(post_state_ == kHttpContextPostInit)
    {
        out_packet_ = packet;
        if(!header_sent_) // 还没有发送chunk header
        {
            post_state_ = kHttpContextPostHttpStreamHeader; 
            connection_->Send(header_.c_str(), header_.size());
            header_sent_ = true;
        }
        else
        {
            post_state_ = kHttpContextPostHttpStreamChunk; // 发送了头就发送len
            connection_->Send(out_packet_->Data(), out_packet_->PacketSize());
        }
        return true;
    }
    return false;
}

/// @brief 发送状态流转的，每次发送结束都会执行这个回调
/// @param  
void HttpContext::WriteComplete(const TcpConnectionPtr &conn)
{
    switch (post_state_)
    {
    case kHttpContextPostInit:
    {
        break;
    }
    case kHttpContextPostHttp:   // http包发送一次就结束了，吧吗 就可以回到初始状态了
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSent(conn);
        break;
    }
    case kHttpContextPostHttpHeader:
    {
        post_state_ = kHttpContextPostHttpBody;
        connection_->Send(out_packet_->Data(), out_packet_->PacketSize());
        break;
    }
    case kHttpContextPostHttpBody:
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSent(conn);
        break;
    }
    case kHttpContextPostChunkHeader:
    {
        post_state_ = kHttpContextPostChunkLen;
        handler_->OnSentNextChunk(conn);
        break;
    }
    case kHttpContextPostChunkLen:
    {
        post_state_ = kHttpContextPostChunkBody;
        connection_->Send(out_packet_->Data(), out_packet_->PacketSize());
        break;
    }
    case kHttpContextPostChunkBody: // chunk发完不一定有下一个，回到初始状态
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSentNextChunk(conn);  // 发送完一个，发下一个
        break;
    }
    case kHttpContextPostChunkEOF:
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSent(conn);
        break;
    }
    case kHttpContextPostHttpStreamHeader:
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSentNextChunk(conn);
        break;
    }
    case kHttpContextPostHttpStreamChunk:
    {
        post_state_ = kHttpContextPostInit;
        handler_->OnSentNextChunk(conn);
        break;
    }
    default:
        break;
    }
}
