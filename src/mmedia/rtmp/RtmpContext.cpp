/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 15:07:06
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:51:31
 */
#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
#include "mmedia/rtmp/amf/AMFObject.h"

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
        auto r = ParseMessage(buf);
        // 解析完成后剩下的数据统计
        last_left_ = buf.ReadableBytes();
        return r;
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
    else if(state_ == kRtmpMessage) // 发送完了检测
    {
        CheckAndSend();
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

    // 上次剩下的数据不使用了，记录当前包的数据大小
    in_bytes_ += (buf.ReadableBytes() - last_left_); // 已经收到的消息字节数
    SendBytesRecv();

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
            memcpy(&timestamp, pos + parsed, 4);  // 小端存放，直接拷贝
            // timestamp = BytesReader::ReadUint32T(pos + parsed);
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
            packet->SetExt(header);     // 消息头存放在ext中
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
    auto type = data->PacketType();
    switch (type)
    {
        case kRtmpMsgTypeChunkSize:
        {
            HandleChunkSize(data);
            break;
        }
        case kRtmpMsgTypeBytesRead:
        {
            RTMP_TRACE << "message bytes read recv;";
            break;
        }
        case kRtmpMsgTypeUserControl:
        {
            HandleUserMessage(data);
            break;
        }
        case kRtmpMsgTypeWindowACKSize:
        {
            HandleAckWindowSize(data);
            break;
        }
        case kRtmpMsgTypeAMF3Message:       // AMF3
        {
            HandleAmfCommand(data, true);
            break;
        }
        case kRtmpMsgTypeAMFMessage:        // AMF0
        {
            HandleAmfCommand(data, false);
            break;
        }
        default:
        RTMP_ERROR << " not surpport message type:" << type;
        break;
    }
}

bool RtmpContext::BuildChunk(const PacketPtr &packet, uint32_t timestamp, bool fmt0)
{
    RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();
    if(header)
    {
        out_sending_packets_.emplace_back(packet);
        RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];   // 取出前一个发送的消息头
        // 1. 不指定fm0  2.没有前一个，第一个肯定是fmt0  3. 当前时间戳大于前一个的就不是fmt0,  当前的msg_sid和前一个一样，肯定不是fmt0
        bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp && header->msg_sid == prev->msg_sid;
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();   // 一开始都是创建新的
        }
        int fmt = kRtmpFmt0;
        if(use_delta) // 不是fmt0，判断其他的
        {
            fmt = kRtmpFmt1; // 至少是1
            timestamp -= prev->timestamp; // 只要不是0 ，timestamp就是差值
            // type和len都相同的至少是2
            if(header->msg_type == prev->msg_type && 
                header->msg_len == prev->msg_len)
            {
                fmt= kRtmpFmt2;
                if(timestamp == out_deltas_[header->cs_id]) // fmt3的deltas和前一个相同，其他的不同
                {
                    fmt = kRtmpFmt3;
                }
            }
        }

        char *p = out_current_;
        if(header->cs_id < 64)  // fmt==2的时候，scid只有6位
        {
            *p++ = (char)((fmt << 6) | header->cs_id);  // 前2位fmt，后6位csid
        }
        else if(header->cs_id < (64 + 256))
        {
            *p++ = (char)((fmt << 6) | 0);      // 第一个字节前2位是fmt，后6位全为0
            *p++ = (char)(header->cs_id - 64);
        }
        else
        {
            *p++ = (char)((fmt << 6) | 1);  
            uint16_t cs = header->cs_id - 64;
            memcpy(p, &cs, sizeof(uint16_t));   // 两个字节，拷贝
            p += sizeof(uint16_t);
        }

        auto ts = timestamp;  
        if(ts >= 0xFFFFFF)      // 有没有使用额外的timestamp
        {
            ts = 0xFFFFFF;
        }

        // 开始组装header数据
        if(fmt == kRtmpFmt0)
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_type); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            
            memcpy(p, &header->msg_sid, 4);     // streamid传输也是小端存放，不需要转换
            p += 4;
            out_deltas_[header->cs_id] = 0;
        }
        else if(fmt == kRtmpFmt1)       // 没有streamid
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_type); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            out_deltas_[header->cs_id] = timestamp;
        }
        else if(fmt == kRtmpFmt2)   // 没有streamid, type, len
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            out_deltas_[header->cs_id] = timestamp;
        }
        // kRtmpFmt3不需要填header

        if(0xFFFFFF == ts)  // 有扩展的，就直接再拷贝四字节
        {
            memcpy(p, &timestamp, 4);
            p += 4;
        }

        // 起始out_current_， 包大小p - out_current_
        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p - out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        prev->cs_id = header->cs_id;    // 更新前一个的值
        prev->msg_len = header->msg_len;
        prev->msg_sid = header->msg_sid;
        prev->msg_type = header->msg_type;
        if(fmt == kRtmpFmt0)    // fmt=0就是当前的时间
        {
            prev->timestamp = timestamp;
        }
        else
        {
            prev->timestamp += timestamp; // fmt！=0前面计算的timestamp是差值
        }

        // packet这个类本身就是存储body的
        const char* body = packet->Data();
        int32_t bytes_parsed = 0;

        // 这里开始是一直发，直到数据包发完为止
        while(true)
        {
            //////////  复制body数据
            const char * chunk = body + bytes_parsed;   // chunk的起始地址
            int32_t size = header->msg_len - bytes_parsed;  // 还剩下多少没解析组装
            size = std::min(size, out_chunk_size_);     

            // header和body分开发送
            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk, size);
            sending_bufs_.emplace_back(std::move(node));
            bytes_parsed += size;

            ///////// 还有数据，不断解析头部，然后循环上面解析body
            if(bytes_parsed < header->msg_len)  // 还没有解析完
            {
                if(out_current_ - out_buffer_ >= 4096) // 超出out_buffer_的容量了
                {
                    RTMP_ERROR << "rtmp had no enough out header buffer.";
                    break;
                }
                // 还有数据， 那就说明是下一个数据包了，还要解析csid
                char *p = out_current_;
                if(header->cs_id < 64)  // fmt==2的时候，scid只有6位
                {
                    // 0xC0 == 11000000 
                    *p++ = (char)((0xC0) | header->cs_id);  // 前2位fmt，后6位csid
                }
                else if(header->cs_id < (64 + 256))
                {
                    *p++ = (char)((0xC0) | 0);      // 第一个字节前2位是fmt，后6位全为0
                    *p++ = (char)(header->cs_id - 64);
                }
                else
                {
                    *p++ = (char)((0xC0) | 1);  
                    uint16_t cs = header->cs_id - 64;
                    memcpy(p, &cs, sizeof(uint16_t));   // 两个字节，拷贝
                    p += sizeof(uint16_t);
                }   

                if(0xFFFFFF == ts)  // 有扩展的，就直接再拷贝四字节
                {
                    memcpy(p, &timestamp, 4);
                    p += 4;
                }

                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p - out_current_);
                sending_bufs_.emplace_back(std::move(nheader)); // sending_bufs_统一用Send发送
                out_current_ = p;
            }
            else        // 数据处理完了，跳出循环
            {
                break;
            }
        }
        return true;
    }  
    return false;
}

void RtmpContext::Send()
{
    if(sending_) // 发送状态中不能再发送，一次一个包
    {
        return;
    }

    sending_ = true;
    for(int i = 0; i < 10; ++i) // 一次最多发送10个包
    {
        if(out_waiting_queue_.empty())  
        {
            break;
        }

        PacketPtr packet = std::move(out_waiting_queue_.front());
        out_waiting_queue_.pop_front();
        // 网络接收的包，拿出来进行组装
        BuildChunk(std::move(packet)); // 拿数据包构建chunk到sending_bufs_
    }

    connection_->Send(sending_bufs_);
}

bool RtmpContext::Ready() const
{
    return !sending_;
}

/// @brief 设置chunkSize参数到packet中
void RtmpContext::SendSetChunkSize()
{
    PacketPtr packet = Packet::NewPacket(64);   // 创建消息包
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>(); // 创建消息头
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;       // 所有控制命令的scid都是2
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeChunkSize; 
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header); // packet设置头
    }

    char *body = packet->Data();
    // out_chunk_size_ 是int32_t类型，占4个字节
    header->msg_len = BytesWriter::WriteUint32T(body, out_chunk_size_);
    packet->SetPacketSize(header->msg_len); // 最大包的大小
    RTMP_DEBUG << "send chunk size:" << out_chunk_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
}

/// @brief 同样，这个包不发送别的数据，单独发送一个ack size
void RtmpContext::SendAckWindowsSize()
{
    PacketPtr packet = Packet::NewPacket(64);   // 创建消息包
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>(); // 创建消息头
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;        
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeWindowACKSize; 
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();
    // out_chunk_size_ 是int32_t类型，占4个字节
    header->msg_len = BytesWriter::WriteUint32T(body, ack_size_);
    packet->SetPacketSize(header->msg_len); 
    RTMP_DEBUG << "send ack size:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
}

/// @brief 
void RtmpContext::SendSetPeerBandwidth()
{
    PacketPtr packet = Packet::NewPacket(64);   // 创建消息包
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>(); // 创建消息头
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;        
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeSetPeerBW; 
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    char *body = packet->Data();
    // ack_size_ 是int32_t类型，占4个字节, 确认窗口大小就是带宽的大小
    body += BytesWriter::WriteUint32T(body, ack_size_);
    *body++ = 0x02; // 设置带宽要多加一个，限制类型，设置为2是动态的
    packet->SetPacketSize(5); // ack_szie 4字节 + 类型 1字节
    RTMP_DEBUG << "send band width:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
}

/// @brief 发送确认消息, 客户端或者服务器在收到窗口大小的字节后必须发送给对端一个确认消息
/// @param bytes 
void RtmpContext::SendBytesRecv()
{
    if(in_bytes_ >= ack_size_)      // 收到的数据已经大于等于确认窗口了
    {
        PacketPtr packet = Packet::NewPacket(64);   // 创建消息包
        RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>(); // 创建消息头
        if(header)
        {
            header->cs_id = kRtmpCSIDCommand;        
            header->msg_len = 0;
            header->msg_type = kRtmpMsgTypeBytesRead;   // 确认消息的类型
            header->timestamp = 0;
            header->msg_sid = kRtmpMsID0;
            packet->SetExt(header);
        }

        char *body = packet->Data();
        // ack_size_ 是int32_t类型
        header->msg_len = BytesWriter::WriteUint32T(body, in_bytes_);
        packet->SetPacketSize(header->msg_len); 
        // RTMP_DEBUG << "send band width:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
        PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
        in_bytes_ = 0;
    }
}

/// @brief  发送控制消息用户控制消息用于告知对方执行该信息中包含的用户控制事件，消息类型为4，并且MesageStreamID使用0，Chumnk StrcamID固定使用2。
/// @param nType  控制事件类型
/// @param value1 
/// @param value2 
void RtmpContext::SendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2)
{
    PacketPtr packet = Packet::NewPacket(64);   // 创建消息包
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>(); // 创建消息头
    if(header)
    {
        header->cs_id = kRtmpCSIDCommand;        
        header->msg_len = 0;
        header->msg_type = kRtmpMsgTypeUserControl; 
        header->timestamp = 0;
        header->msg_sid = kRtmpMsID0;
        packet->SetExt(header);
    }

    // 返回的是packet类之后的第一个地址位置
    char *body = packet->Data();
    char *p = body;
    p += BytesWriter::WriteUint16T(body, nType);    // event type
    p += BytesWriter::WriteUint32T(body, value1);   // event data
    if(nType == kRtmpEventTypeSetBufferLength)  // 设置buffer长度
    {
         p += BytesWriter::WriteUint32T(body, value2);
    }
    RTMP_DEBUG << "send control type:" << nType 
                << " value1:" << value1 
                << " value2:" << value2
                << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
}

/// @brief 接收数据，处理控制消息,chunkSize
/// @param packet 
void RtmpContext::HandleChunkSize(PacketPtr &packet)
{
    if(packet->PakcetSize() >= 4) // chunkSize至少4字节
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv chunk size in_chunk_size:" << in_chunk_size_ << " change to:" << size;
        in_chunk_size_ = size;
    }
    else
    {
        RTMP_ERROR << "invalid chunk size packet msg_len:" << packet->PakcetSize()
            << " host :" << connection_->PeerAddr().ToIpPort();
    }
}

void RtmpContext::HandleAckWindowSize(PacketPtr &packet)
{
    if(packet->PakcetSize() >= 4) // windowSize至少4字节
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv ack window size ack_size_:" << ack_size_ << " change to:" << size;
        in_chunk_size_ = size;
    }
    else
    {
        RTMP_ERROR << "invalid ack window size packet msg_len:" << packet->PakcetSize()
         << " host :" << connection_->PeerAddr().ToIpPort();
    }
}

/// @brief 用户控制消息解析
/// @param packet 
void RtmpContext::HandleUserMessage(PacketPtr &packet)
{
    auto msg_len = packet->PakcetSize();
    if(msg_len < 6) // 2字节的type + 4字节自定义数据 最少
    {
        RTMP_ERROR << "invalid user control packet msg_len:" << packet->PakcetSize()
         << " host :" << connection_->PeerAddr().ToIpPort();
        return;
    }

    char* body = packet->Data();
    // 1.解析类型
    auto type = BytesReader::ReadUint16T(body);
    body += 2;
    // 2.解析用户数据 4字节
    auto value = BytesReader::ReadUint32T(body);

    RTMP_TRACE << "recv user control type:" << value << " host :" << connection_->PeerAddr().ToIpPort();
    switch (type)
    {
        case kRtmpEventTypeStreamBegin:      // 流开始
        {
            RTMP_TRACE << "recv stream begin value:" << value << " host :" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypeStreamEOF:      // 流结束
        {
            RTMP_TRACE << "recv stream eof value:" << value << " host :" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypeStreamDry:     // 流变为空（无数据）
        {
            RTMP_TRACE << "recv stream dry value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypeSetBufferLength:     // 设置缓冲长度
        {
            RTMP_TRACE << "recv set buffer length value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            if(msg_len < 10)
            {
                RTMP_ERROR << "invalid user control packet msg_len:" << packet->PakcetSize()
                << " host:" << connection_->PeerAddr().ToIpPort();                
                return ;
            }
            break;
        }
        case kRtmpEventTypeStreamsRecorded:     // 录制的流
        {
            RTMP_TRACE << "recv stream recoded value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        case kRtmpEventTypePingRequest:     // Ping 请求
        {
            RTMP_TRACE << "recv ping request value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            SendUserCtrlMessage(kRtmpEventTypePingResponse,value,0);
            break;
        }
        case kRtmpEventTypePingResponse:      // Ping 响应
        {
            RTMP_TRACE << "recv ping response value" << value << " host:" << connection_->PeerAddr().ToIpPort();
            break;
        }
        default:
            break;
    }
}

void RtmpContext::HandleAmfCommand(PacketPtr &data, bool amf3)
{
    RTMP_TRACE << "amf message len:" << data->PakcetSize() << " host:" << connection_->PeerAddr().ToIpPort();

    const char *body = data->Data();
    int32_t msg_len = data->PakcetSize();

    // amf3在开头多了一个字节，表示是amf3，剩下的就是amf0
    if(amf3)
    {
        body += 1;
        msg_len -= 1;
    }

    AMFObject obj;
    if(obj.Decode(body, msg_len) < 0)
    {
        RTMP_ERROR << "amf decode failed. host:" << connection_->PeerAddr().ToIpPort();
        return;
    }
    obj.Dump();

}

bool RtmpContext::BuildChunk(PacketPtr &&packet, uint32_t timestamp, bool fmt0)
{
    RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();
    if(header)
    {
        out_sending_packets_.emplace_back(std::move(packet));
        RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];   // 取出前一个发送的消息头
        // 1. 不指定fm0  2.没有前一个，第一个肯定是fmt0  3. 当前时间戳大于前一个的就不是fmt0,  当前的msg_sid和前一个一样，肯定不是fmt0
        bool use_delta = !fmt0 && !prev && timestamp >= prev->timestamp && header->msg_sid == prev->msg_sid;
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();   // 一开始都是创建新的
        }
        int fmt = kRtmpFmt0;
        if(use_delta) // 不是fmt0，判断其他的
        {
            fmt = kRtmpFmt1; // 至少是1
            timestamp -= prev->timestamp; // 只要不是0 ，timestamp就是差值
            // type和len都相同的至少是2
            if(header->msg_type == prev->msg_type && 
                header->msg_len == prev->msg_len)
            {
                fmt= kRtmpFmt2;
                if(timestamp == out_deltas_[header->cs_id]) // fmt3的deltas和前一个相同，其他的不同
                {
                    fmt = kRtmpFmt3;
                }
            }
        }

        char *p = out_current_;
        if(header->cs_id < 64)  // fmt==2的时候，scid只有6位
        {
            *p++ = (char)((fmt << 6) | header->cs_id);  // 前2位fmt，后6位csid
        }
        else if(header->cs_id < (64 + 256))
        {
            *p++ = (char)((fmt << 6) | 0);      // 第一个字节前2位是fmt，后6位全为0
            *p++ = (char)(header->cs_id - 64);
        }
        else
        {
            *p++ = (char)((fmt << 6) | 1);  
            uint16_t cs = header->cs_id - 64;
            memcpy(p, &cs, sizeof(uint16_t));   // 两个字节，拷贝
            p += sizeof(uint16_t);
        }

        auto ts = timestamp;  
        if(ts >= 0xFFFFFF)      // 有没有使用额外的timestamp
        {
            ts = 0xFFFFFF;
        }

        // 开始组装header数据
        if(fmt == kRtmpFmt0)
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_type); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            
            memcpy(p, &header->msg_sid, 4);     // streamid传输也是小端存放，不需要转换
            p += 4;
            out_deltas_[header->cs_id] = 0;
        }
        else if(fmt == kRtmpFmt1)       // 没有streamid
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_type); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            out_deltas_[header->cs_id] = timestamp;
        }
        else if(fmt == kRtmpFmt2)   // 没有streamid, type, len
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            out_deltas_[header->cs_id] = timestamp;
        }
        // kRtmpFmt3不需要填header

        if(0xFFFFFF == ts)  // 有扩展的，就直接再拷贝四字节
        {
            memcpy(p, &timestamp, 4);
            p += 4;
        }

        // 起始out_current_， 包大小p - out_current_
        BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p - out_current_);
        sending_bufs_.emplace_back(std::move(nheader));
        out_current_ = p;

        prev->cs_id = header->cs_id;    // 更新前一个的值
        prev->msg_len = header->msg_len;
        prev->msg_sid = header->msg_sid;
        prev->msg_type = header->msg_type;
        if(fmt == kRtmpFmt0)    // fmt=0就是当前的时间
        {
            prev->timestamp = timestamp;
        }
        else
        {
            prev->timestamp += timestamp; // fmt！=0前面计算的timestamp是差值
        }

        // packet这个类本身就是存储body的
        const char* body = packet->Data();
        int32_t bytes_parsed = 0;

        // 这里开始是一直发，直到数据包发完为止
        while(true)
        {
            //////////  复制body数据
            const char * chunk = body + bytes_parsed;   // chunk的起始地址
            int32_t size = header->msg_len - bytes_parsed;  // 还剩下多少没解析组装
            size = std::min(size, out_chunk_size_);     

            // header和body分开发送
            BufferNodePtr node = std::make_shared<BufferNode>((void*)chunk, size);
            sending_bufs_.emplace_back(std::move(node));
            bytes_parsed += size;

            ///////// 还有数据，不断解析头部，然后循环上面解析body
            if(bytes_parsed < header->msg_len)  // 还没有解析完
            {
                if(out_current_ - out_buffer_ >= 4096) // 超出out_buffer_的容量了
                {
                    RTMP_ERROR << "rtmp had no enough out header buffer.";
                    break;
                }
                // 还有数据， 那就说明是下一个数据包了，还要解析csid
                char *p = out_current_;
                if(header->cs_id < 64)  // fmt==2的时候，scid只有6位
                {
                    // 0xC0 == 11000000 
                    *p++ = (char)((0xC0) | header->cs_id);  // 前2位fmt，后6位csid
                }
                else if(header->cs_id < (64 + 256))
                {
                    *p++ = (char)((0xC0) | 0);      // 第一个字节前2位是fmt，后6位全为0
                    *p++ = (char)(header->cs_id - 64);
                }
                else
                {
                    *p++ = (char)((0xC0) | 1);  
                    uint16_t cs = header->cs_id - 64;
                    memcpy(p, &cs, sizeof(uint16_t));   // 两个字节，拷贝
                    p += sizeof(uint16_t);
                }   

                if(0xFFFFFF == ts)  // 有扩展的，就直接再拷贝四字节
                {
                    memcpy(p, &timestamp, 4);
                    p += 4;
                }

                BufferNodePtr nheader = std::make_shared<BufferNode>(out_current_, p - out_current_);
                sending_bufs_.emplace_back(std::move(nheader)); // sending_bufs_统一用Send发送
                out_current_ = p;
            }
            else        // 数据处理完了，跳出循环
            {
                break;
            }
        }
        return true;
    }  
    return false;
}

void RtmpContext::CheckAndSend()
{
    sending_ = false;
    out_current_ = out_buffer_;
    sending_bufs_.clear();
    out_sending_packets_.clear();

    if(!out_waiting_queue_.empty())     // 还有数据
    {
        Send();
    }
    else
    {
        if(rtmp_handler_)   // 发送完了，让上层处理
        {
            rtmp_handler_->OnActive(connection_);
        }
    }
}

void RtmpContext::PushOutQueue(PacketPtr &&packet)
{
    out_waiting_queue_.push_back(std::move(packet));
    Send();
}

