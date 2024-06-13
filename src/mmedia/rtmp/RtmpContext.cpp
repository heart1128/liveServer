/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 15:07:06
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-13 10:13:32
 */
#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

RtmpContext::RtmpContext(const TcpConnectionPtr &con, RtmpHandler *handler, bool client)
:handshake_(con, client), connection_(con), rtmp_handler_(handler)
{
} 

/// @brief 状态机解析rtmp头
/// @param buf 
/// @return 
int32_t RtmpContext::Parse(MsgBuffer &buf)
{
    int32_t ret = 0;
    if(state_ == kRtmpHandShake)        // 正在握手状态，开始握手
    {
        ret = handshake_.HandShake(buf);
        if(ret == 0)    // 成功握手
        {
            state_ = kRtmpMessage;      // 状态改变，可以接受消息
            if(buf.ReadableBytes() > 0)
            {
                return Parse(buf);  // 继续解析，到kRtmpMessage解析
            }
        }
        else if(ret == -1)
        {
            RTMP_TRACE << "rtmp handshake error!";
            // RTMP_ERROR << "rtmp handshake error!";
        }
        else if(ret == 2)   // 中间状态.收到了s1s2s0
        {
            state_ = kRtmpWatingDone;
        }
    }
    else if(state_ == kRtmpMessage)
    {
        return ParseMessage(buf);
    }

    return ret;
}

/// @brief 调用握手的writercomplete回调函数，握手
void RtmpContext::OnWriteComplete()
{
    if(state_ == kRtmpHandShake)
    {
        handshake_.WriteComplete();
    }
    else if(state_ == kRtmpWatingDone)
    {
        state_ = kRtmpMessage;
    }
    else if(state_ == kRtmpMessage)
    {

    }
}

/// @brief  启动握手，调用handshake_类的开始，客户端开始发送C0C1，服务端等待
void RtmpContext::StarHandShake()
{
    handshake_.Start();
}

/// @brief 解析rtmp消息包，header + body
/// @param buf 
/// @return 1:数据不足；
int32_t RtmpContext::ParseMessage(MsgBuffer &buf)
{
    uint8_t fmt;    // 一个字节
    uint32_t csid, msg_len = 0, msg_sid = 0, timestamp = 0;
    uint8_t msg_type = 0;
    uint32_t total_bytes = buf.ReadableBytes(); // 包的大小
    int32_t parsed = 0;     // 已经解析的字节数

    while(total_bytes > 1)
    {
        const char *pos = buf.Peek();

        //*********basic header********* */ 
        fmt = (*pos >> 6)&0x03;     // fmt占前两位，一共表示0，1，2，3
        csid = *pos & 0x3F;         // fmt后6位
        parsed++;

        // csid 取0，1是一个新的消息包，如果是2，就是使用上一个包的所有东西，不用更新csid
        if(csid == 0)           // csid取0，表示basic header长度是2bytes
        {
            if(total_bytes < 2) // 不足，等待数据
            {
                return 1;
            }

            csid = 64;  // csid的前六位是0，表示csid(chunk stream id)最少是64，
            csid += *((uint8_t*)(pos + parsed)); // 真正csid的值
            parsed++;
        }
        else if(csid == 1)      // csid =1 除了fmt的后6位全为1，表示basic header长度为3字节
        {
            if(total_bytes < 3)
            {
                return 1;
            }

            csid = 64;  // csid的前六位是1，表示csid(chunk stream id)最少是64，
            csid += *((uint8_t*)(pos + parsed)); // csid的第一个字节
            parsed++;
            csid += *((uint8_t*)(pos + parsed)) * 256; // 因为前面加了8位了，后面的8位是从256开始的2^8 = 256
            parsed++;
        }

        int size = total_bytes - parsed;
        // fmt表示整个message header的长度，剩下的数据不够就等待
        if(size == 0 || (fmt == 0 && size < 11) 
            || (fmt == 1 && size < 7) || (fmt == 2 && size < 3))
        {
            return 1;
        }


        //*********message header********* */ 
        msg_len = 0;
        msg_sid = 0;
        msg_type = 0;
        timestamp = 0;
        int32_t ts = 0;

        RtmpMsgHeaderPtr &prev = in_message_headers_[csid]; // 取出csid对应的header
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }

        if(fmt == kRtmpFmt0)
        {
            // timestamp 是前3字节
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            in_deltas_[csid] = 0;   // fmt=0的时候，存的timestamp是绝对时间，所以时间差为0。其他两个存的是和上一个chunk的时间差
            timestamp = ts;
            msg_len = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            msg_type = BytesReader::ReadUint8T(pos + parsed);
            parsed += 1;
            msg_sid = BytesReader::ReadUint32T(pos + parsed);
            parsed += 4;
        }
        else if(fmt == kRtmpFmt1)
        {
            // timestamp 是前3字节
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            in_deltas_[csid] = ts;   // 时间差
            timestamp = ts + prev->timestamp;
            msg_len = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            msg_type = BytesReader::ReadUint8T(pos + parsed);
            parsed += 1;
            // fmt=1用前一个消息的stream id
            msg_sid = prev->msg_sid;
        }
        else if(fmt == kRtmpFmt2)
        {
            // timestamp 是前3字节
            ts = BytesReader::ReadUint24T(pos + parsed);
            parsed += 3;
            in_deltas_[csid] = ts;   
            timestamp = ts + prev->timestamp;
            // fmt=2，下面都用前一个
            msg_len = prev->msg_len;
            msg_type = prev->msg_type;
            msg_sid = prev->msg_sid;
        }
        else if(fmt == kRtmpFmt3)    // 3都是用其哪一个的，就csid不一样
        { 
            timestamp = in_deltas_[csid] + prev->timestamp;
            msg_len = prev->msg_len;
            msg_type = prev->msg_type;
            msg_sid = prev->msg_sid;
        }

        //*********Ext timestamp********* */ 
        bool ext = (ts == 0xFFFFFF); // timestamp全1表示有ext
        if(fmt == kRtmpFmt3)        // fmt=3，都是用的前一个header,直接沿用就行
        {
            ext = in_ext_[csid];
        }
        in_ext_[csid] = ext;
        if(ext)
        {
            if(total_bytes - parsed < 4)
            {
                return 1;
            }
            timestamp = BytesReader::ReadUint32T(pos + parsed);
            parsed += 4;
            if(fmt != kRtmpFmt0)    // 不是0，要计算时间差
            {
                timestamp = ts + prev->timestamp;
                in_deltas_[csid] = ts;
            }
        }


        // 数据包
        PacketPtr &packet = in_packets_[csid];
        if(!packet)
        {
            packet = Packet::NewPacket(msg_len);
        }
        RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>(); // 创建数据包头
        if(!header)
        {
            header = std::make_shared<RtmpMsgHeader>();
            packet->SetExt(header);
        }

        header->cs_id = csid;
        header->msg_len = msg_len;
        header->msg_sid = msg_sid;
        header->msg_type = msg_type;
        header->timestamp = timestamp;

        // 取body数据
        int bytes = std::min(packet->Space(), in_chunk_size_);
        if(total_bytes - parsed < bytes)
        {
            return 1;
        }

        // 存body数据的起始位置
        const char *body = packet->Data() + packet->PakcetSize();
        memcpy((void*)body, pos + parsed, bytes);
        packet->UpdatePacketSize(bytes);
        parsed += bytes;

        buf.Retrieve(parsed);
        total_bytes -= parsed;
        
        prev->cs_id = csid;
        prev->msg_len = msg_len;
        prev->msg_sid = msg_sid;
        prev->msg_type = msg_type;
        prev->timestamp = timestamp;

        if(packet->Space() == 0)    // 消息处理完了
        {
            packet->SetPacketType(msg_type);
            packet->SetTimeStamp(timestamp);
            MessageComplete(std::move(packet));
            packet.reset();
        }
    }
    return 1;
}

void RtmpContext::MessageComplete(PacketPtr &&data)
{
    RTMP_TRACE << "recv message type : " << data->PacketType() << " len : " << data->PakcetSize() << std::endl;
}
