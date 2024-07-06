#include "FlvContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/rtmp/RtmpHeader.h"
#include <sstream>

using namespace tmms::mm;

// flv只音频频的 flvheader
static char flv_audio_only_header[] =
{
    0x46,  // 'F'
    0x4c,  // 'L'
    0x56,  // 'V'
    0x01,  // version = 1
    0x04,  // 00000100 只有音频，音频在第6位
    0x00,
    0x00,
    0x00,
    0x09, // header size
};

// flv只有音频的 flvheader
static char flv_video_only_header[] =
{
    0x46,  // 'F'
    0x4c,  // 'L'
    0x56,  // 'V'
    0x01,  // version = 1
    0x01,  // 00000001 只有视频，视频在第8位
    0x00,
    0x00,
    0x00,
    0x09, // header size
};

// flv 音视频都有 flvheader
static char flv_header[] =
{
    0x46,  // 'F'
    0x4c,  // 'L'
    0x56,  // 'V'
    0x01,  // version = 1
    0x05,  // 00000101 
    0x00,
    0x00,
    0x00,
    0x09, // header size
};


FlvContext::FlvContext(const TcpConnectionPtr &conn, MMediaHandler *handler)
:connection_(conn), handler_(handler)
{
    current_ = out_buffer_;
}

/// @brief 做http头 -> flv头 -> 发送
/// @param has_video 
/// @param has_audio 
void FlvContext::SendHttpHeader(bool has_video, bool has_audio)
{
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK \r\n";
    ss << "Access-Control-Allow-Origin: *\r\n";  // 标识可以跨域，允许一个网域下的网页访问另一个网域下的资源，而不违反同源策略。
    ss << "Content-Type: video/x-flv\r\n";
    ss << "Connection: Keep-Alive\r\n";
    ss << "\r\n";

    http_header_ = std::move(ss.str());
    auto header_node = std::make_shared<BufferNode>((void*)http_header_.data(), http_header_.size());
    bufs_.emplace_back(std::move(header_node));
    WriteFlvHeader(has_video, has_audio);
    RTMP_DEBUG << " SendHttpHeader. sending_:" << sending_;
    Send();
}

/// @brief 制作flv头
/// @param has_video 
/// @param has_audio 
void FlvContext::WriteFlvHeader(bool has_video, bool has_audio)
{
    char *header = current_;
    if(!has_audio)  // 只有视频
    {
        memcpy(current_, flv_video_only_header,sizeof(flv_video_only_header));
        current_ += sizeof(flv_video_only_header);
    }
    else if(!has_video) // 只有音频
    {
        memcpy(current_, flv_audio_only_header,sizeof(flv_audio_only_header));
        current_ += sizeof(flv_audio_only_header);
    }
    else    // 都有
    {
        memcpy(current_, flv_header,sizeof(flv_header));
        current_ += sizeof(flv_header);
    }
    
    auto h = std::make_shared<BufferNode>(header, current_ - header);
    bufs_.emplace_back(std::move(h));
}

/// @brief perviours size + [tag header, tag body]
/// @param pkt 
/// @param timestamp 
/// @return 
bool FlvContext::BuildFlvFrame(PacketPtr &pkt, uint32_t timestamp)
{
    out_packets_.emplace_back(pkt);
    char *header = current_;
    char *p = (char*)&previous_size_;

    // 小端转大端，大的在小地址
    *current_++ = p[3];
    *current_++ = p[2];
    *current_++ = p[1];
    *current_++ = p[0];

    // tag header
        // tag type 1 byte
    *current_++ = GetRtmpPacketType(pkt);
        // datasize 3 bytes
    auto mlen = pkt->PacketSize();
    p = (char*)&mlen;
    *current_++ = p[2];
    *current_++ = p[1];
    *current_++ = p[0];
        // timestamp 3 bytes
    p = (char*)&timestamp;
    *current_++ = p[2];
    *current_++ = p[1];
    *current_++ = p[0];
        // timestampext
    *current_++ = 0; // 固定0
        // stream id 3 bytes 
    *current_++ = 0;
    *current_++ = 0;
    *current_++ = 0;

    previous_size_ = mlen + 11;  // 更新上一个长度 header + body 头固定11

    auto h = std::make_shared<BufferNode>(header, current_ - header);
    bufs_.emplace_back(std::move(h));   // tag header 和body是分开发的

    // tag body
    auto c = std::make_shared<BufferNode>(pkt->Data(), pkt->PacketSize());
    bufs_.emplace_back(std::move(c));
    return true;
}

void FlvContext::Send()
{
    if(sending_)
    {
        return;
    }

    sending_ = true;
    connection_->Send(bufs_);
}

/// @brief 数据写完成了，清除所有状态, 通知激活，进行业务流转
/// @param conn 
void FlvContext::WriterComplete(const TcpConnectionPtr &conn)
{
    sending_ = false;
    current_ = out_buffer_;
    bufs_.clear();
    out_packets_.clear();

    // 通知业务层
    if(handler_)
    {
        handler_->OnActive(conn);
    }
}

bool FlvContext::Ready() const
{
    return sending_;
}

/// @brief rtmp推流上来的数据类型，要转成flv的类型
/// @param pkt 
/// @return 0:错误的包
char FlvContext::GetRtmpPacketType(PacketPtr &pkt)
{
    if(pkt->IsAudio())
    {
        return kRtmpMsgTypeAudio;
    }
    else if(pkt->IsVideo())
    {
        return kRtmpMsgTypeVideo;
    }
    else if(pkt->IsMeta())
    {
        return kRtmpMsgTypeAMFMeta;
    }

    return 0;
}
