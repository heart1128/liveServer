/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 15:07:06
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-30 12:19:15
 */
#include "RtmpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
#include "mmedia/rtmp/amf/AMFObject.h"
#include "base/StringUtils.h"
#include <sstream>

using namespace tmms::mm;

RtmpContext::RtmpContext(const TcpConnectionPtr &con, RtmpHandler *handler, bool client)
:handshake_(con, client), connection_(con), rtmp_handler_(handler), is_client_(client)
{
    commands_["connect"] = std::bind(&RtmpContext::HandleConnect, this, std::placeholders::_1);
    commands_["createStream"] = std::bind(&RtmpContext::HandleCreateStream, this, std::placeholders::_1);
    commands_["_result"] = std::bind(&RtmpContext::HandleResult, this, std::placeholders::_1);
    commands_["_error"] = std::bind(&RtmpContext::HandleError, this, std::placeholders::_1);
    commands_["play"] = std::bind(&RtmpContext::HandlePlay, this, std::placeholders::_1);
    commands_["publish"] = std::bind(&RtmpContext::HandlePublish, this, std::placeholders::_1);
    out_current_ = out_buffer_;
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
            // 客户端可以发送连接了
            if(is_client_)
            {
                SendConnect();
            }
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
        // 可能是在中途完成了握手
        if(is_client_)
        {
            SendConnect();
        }
    }
    else if(state_ == kRtmpMessage) // 发送完了检测
    {
        CheckAndSend();
    }
}

/// @brief  启动握手，调用handshake_类的开始，客户端开始发送C0C1，服务端等待
void RtmpContext::StartHandShake()
{
    handshake_.Start();
}

/// @brief 解析rtmp消息包，header + body
/// @param buf 
/// @return 1:数据不足；
int32_t RtmpContext::ParseMessage(MsgBuffer &buf)
{
    uint8_t fmt;
    uint32_t csid,msg_len=0,msg_sid=0;
    uint8_t msg_type = 0;
    uint32_t total_bytes = buf.ReadableBytes();
    int32_t parsed = 0;

    in_bytes_ += (buf.ReadableBytes()-last_left_);
    SendBytesRecv();
    
    while(total_bytes>1)
    {
        const char *pos = buf.Peek();
        parsed = 0;
        //Basic Header
        fmt = (*pos>>6)&0x03;
        csid = *pos&0x3F;
        parsed++;

        if(csid == 0)
        {
            if(total_bytes<2)
            {
                return 1;
            }
            csid = 64;
            csid += *((uint8_t*)(pos+parsed));
            parsed++;
        }
        else if( csid == 1)
        {
            if(total_bytes<3)
            {
                return 1;
            }
            csid = 64;
            csid += *((uint8_t*)(pos+parsed));
            parsed++;
            csid +=  *((uint8_t*)(pos+parsed))*256;
            parsed ++;           
        }

        int size = total_bytes - parsed;
        if(size == 0
            ||(fmt==0&&size<11)
            ||(fmt==1&&size<7)
            ||(fmt==2&&size<3))
        {
            return 1;
        }

        msg_len = 0;
        msg_sid = 0;
        msg_type = 0;
        int32_t ts = 0;

        RtmpMsgHeaderPtr &prev = in_message_headers_[csid];
        if(!prev)
        {
            prev = std::make_shared<RtmpMsgHeader>();
        }
        msg_len = prev->msg_len;
        if(fmt == kRtmpFmt0 || fmt == kRtmpFmt1)
        {
            msg_len = BytesReader::ReadUint24T((pos+parsed)+3);
        }
        else if(msg_len == 0)
        {
            msg_len = in_chunk_size_;
        }
        PacketPtr &packet = in_packets_[csid];
        if(!packet)
        {
            packet = Packet::NewPacket(msg_len);
            RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
            header->cs_id = csid;
            header->msg_len = msg_len;
            header->msg_sid = msg_sid;
            header->msg_type = msg_type;
            header->timestamp = 0;  
            packet->SetExt(header);          
        }

        RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();

        if(fmt == kRtmpFmt0)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = 0;
            header->timestamp = ts;
            header->msg_len = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            header->msg_type = BytesReader::ReadUint8T(pos+parsed);
            parsed += 1;
            memcpy(&header->msg_sid,pos+parsed,4);
            parsed += 4;
        }
        else if(fmt == kRtmpFmt1)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = ts;
            header->timestamp = ts + prev->timestamp;
            header->msg_len = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            header->msg_type = BytesReader::ReadUint8T(pos+parsed);
            parsed += 1;
            header->msg_sid = prev->msg_sid;
        }
        else if(fmt == kRtmpFmt2)
        {
            ts = BytesReader::ReadUint24T(pos+parsed);
            parsed += 3;
            in_deltas_[csid] = ts;
            header->timestamp = ts + prev->timestamp;
            header->msg_len = prev->msg_len;
            header->msg_type = prev->msg_type;
            header->msg_sid = prev->msg_sid;
        }    
        else if(fmt == kRtmpFmt3)
        {
            if(header->timestamp == 0)
            {
                header->timestamp = in_deltas_[csid] + prev->timestamp;
            }
            header->msg_len = prev->msg_len;
            header->msg_type = prev->msg_type;
            header->msg_sid = prev->msg_sid;
        } 

        bool ext = (ts == 0xFFFFFF);
        if(fmt == kRtmpFmt3)
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
            ts = BytesReader::ReadUint32T(pos+parsed);
            parsed += 4;
            if(fmt != kRtmpFmt0)
            {
                header->timestamp = ts+ prev->timestamp;
                in_deltas_[csid] = ts;
            }
        }

        int bytes = std::min(packet->Space(),in_chunk_size_);
        if(total_bytes - parsed < bytes)
        {
            return 1;
        }

        const char * body = packet->Data() + packet->PacketSize();
        memcpy((void*)body,pos+parsed,bytes);
        packet->UpdatePacketSize(bytes);
        parsed += bytes;
        
        buf.Retrieve(parsed);
        total_bytes -= parsed;

        prev->cs_id = header->cs_id;
        prev->msg_len = header->msg_len;
        prev->msg_sid = header->msg_sid;
        prev->msg_type = header->msg_type;
        prev->timestamp = header->timestamp;

        if(packet->Space() == 0)
        {
            packet->SetPacketType(header->msg_type);
            packet->SetTimeStamp(header->timestamp);
            MessageComplete(std::move(packet));
            packet.reset();
        }
    }
    return 1;
}

/// @brief 解析完成数据之后，对应的命令控制类型消息进行处理，对应的音视频数据给业务层转发
/// @param data 
void RtmpContext::MessageComplete(PacketPtr &&data)
{
    // RTMP_TRACE << "recv message type : " << data->PacketType() << " len : " << data->PacketSize() << std::endl;
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
        case kRtmpMsgTypeAMFMeta: // 都是一样的转换视频
        case kRtmpMsgTypeAMF3Meta:
        case kRtmpMsgTypeAudio:  // 音频
        case kRtmpMsgTypeVideo:  // 视频
        {
            SetPacketType(data); // 转换类型
            // 给业务层包，数据已经解析过了
            if(rtmp_handler_)
            {
                rtmp_handler_->OnRecv(connection_, std::move(data));
            }
            break;
        }
        default:
            RTMP_TRACE << " not surpport message type:" << type;
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
            p += BytesWriter::WriteUint24T(p, header->msg_len); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            
            memcpy(p, &header->msg_sid, 4);     // streamid传输也是小端存放，不需要转换
            p += 4;
            out_deltas_[header->cs_id] = 0;
        }
        else if(fmt == kRtmpFmt1)       // 没有streamid
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_len); // msg_len 3字节
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
    //RTMP_TRACE << "send chunk size:" << out_chunk_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));  // 加入需要build包的数据队列中，等待build，然后send
}

/// @brief 同样，这个包不发送别的数据，单独发送一个ack size
void RtmpContext::SendAckWindowSize()
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
    // RTMP_TRACE << "send ack size:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
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
    header->msg_len = 5;
    packet->SetPacketSize(5); // ack_szie 4字节 + 类型 1字节
    RTMP_DEBUG << "send band width:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
    //RTMP_TRACE << "send band width:" << ack_size_ << " to host:" << connection_->PeerAddr().ToIpPort();
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
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
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
    if(packet->PacketSize() >= 4) // chunkSize至少4字节
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv chunk size in_chunk_size:" << in_chunk_size_ << " change to:" << size;
        in_chunk_size_ = size;
    }
    else
    {
        RTMP_ERROR << "invalid chunk size packet msg_len:" << packet->PacketSize()
            << " host :" << connection_->PeerAddr().ToIpPort();
    }
}

void RtmpContext::HandleAckWindowSize(PacketPtr &packet)
{
    if(packet->PacketSize() >= 4) // windowSize至少4字节
    {
        auto size = BytesReader::ReadUint32T(packet->Data());
        RTMP_DEBUG << "recv ack window size ack_size_:" << ack_size_ << " change to:" << size;
        in_chunk_size_ = size;
    }
    else
    {
        RTMP_ERROR << "invalid ack window size packet msg_len:" << packet->PacketSize()
         << " host :" << connection_->PeerAddr().ToIpPort();
    }
}

/// @brief 用户控制消息解析
/// @param packet 
void RtmpContext::HandleUserMessage(PacketPtr &packet)
{
    auto msg_len = packet->PacketSize();
    if(msg_len < 6) // 2字节的type + 4字节自定义数据 最少
    {
        RTMP_ERROR << "invalid user control packet msg_len:" << packet->PacketSize()
         << " host :" << connection_->PeerAddr().ToIpPort();
        return;
    }

    char* body = packet->Data();
    // 1.解析类型
    auto type = BytesReader::ReadUint16T(body);
    // 2.解析用户数据 4字节
    auto value = BytesReader::ReadUint32T(body + 2);

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
                RTMP_ERROR << "invalid user control packet msg_len:" << packet->PacketSize()
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

/// @brief 所有的rtmp命令消息都用amf封装，从amf中拿出命令，调用对应处理函数
/// @param data 
/// @param amf3 
void RtmpContext::HandleAmfCommand(PacketPtr &data, bool amf3)
{
    RTMP_TRACE << "amf message len:" << data->PacketSize() << " host:" << connection_->PeerAddr().ToIpPort();

    const char *body = data->Data();
    int32_t msg_len = data->PacketSize();

    // amf3在开头多了一个字节，表示是amf3，剩下的就是amf0
    if(amf3)
    {
        body += 1;
        msg_len -= 1;
    }

    AMFObject obj;
    // 解析数据包里面所有的数据类10.144.233.101型，然后加入peoperty中
    if(obj.Decode(body, msg_len) < 0)
    {
        RTMP_ERROR << "amf decode failed. host:" << connection_->PeerAddr().ToIpPort();
        return;
    }
   // obj.Dump();

    // 获取消息命令名称，在封装的时候把name写在了amf的第一个
    const std::string &method = obj.Property(0)->String();
    RTMP_TRACE << "amf command:" << method << " host:" << connection_->PeerAddr().ToIpPort();

    auto iter = commands_.find(method);
    if(iter == commands_.end()) // 不支持这个命令
    {
        RTMP_TRACE << "not surpport method:" << method << " host:" << connection_->PeerAddr().ToIpPort();
        return;
    }

    // 找到，启动命令函数
    iter->second(obj);

}

/// @brief 发送connect的控制命令，带有描述信息
void RtmpContext::SendConnect()
{
    SendSetChunkSize(); // 设置chunksize，每个控制命令都是一个packet
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;    // 使用AMF的初始化，固定为3
    header->msg_sid = 0;        // 固定的值在connect中
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; // AMF0，消息使用AMF序列化
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    // 设置消息体
    p += AMFAny::EncodeString(p, "connect");    // command Name
    p += AMFAny::EncodeNumber(p, 1.0);  // Transaction ID 固定的1
    *p++ = kAMFObject;
    // command object
    p += AMFAny::EncodeNamedString(p, "app", app_);     // 推流点
    p += AMFAny::EncodeNamedString(p, "tcUrl", tc_url_);    // 推流url
    p += AMFAny::EncodeNamedBoolean(p, "fpad", false); // 
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31.0);
    p += AMFAny::EncodeNamedNumber(p, "audioCodecs", 1639.0);   // 可以支持的音频类型，二进制1表示
    p += AMFAny::EncodeNamedNumber(p, "videoCodecs", 252.0);     // 同样可支持的视频编码类型
    p += AMFAny::EncodeNamedNumber(p, "videoFunction", 1.0);
    *p++ = 0x00;
    *p++ = 0x00;        // AMF
    *p++ = 0x09;

    // 设置包长度
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "send connect msg_len:" << header->msg_len << " host:" << connection_->PeerAddr().ToIpPort();
    
    // 加入到待编码的队列中
    PushOutQueue(std::move(packet));
}

/// @brief 作为服务端使用，收到了上面的请求格式消息包，要组装回应消息包
/// @param obj 
void RtmpContext::HandleConnect(AMFObject &obj)
{
    auto amf3 = false;
    tc_url_ = obj.Property("tcUrl")->String();
    // 获取command object，在第二个位置（0开始）
    AMFObjectPtr sub_obj = obj.Property(2)->Object();
    if(sub_obj)
    {
        app_ = sub_obj->Property("app")->String();
        if(sub_obj->Property("objectEncoding"))
        {
            amf3 = sub_obj->Property("objectEncoding")->Number() == 3.0;
        }
    }

    RTMP_TRACE << "recv connect tcUrl:" << tc_url_ << " app:" << app_ << " amf3:" << amf3;
    // 三个过程都是设置好的
    SendAckWindowSize();
    SendSetPeerBandwidth(); // 设置带宽
    SendSetChunkSize(); // 设置chunkSize

    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;    // 使用AMF的初始化，固定为3
    header->msg_sid = 0;        // 固定的值在connect中
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; // AMF0，消息使用AMF序列化
    packet->SetExt(header); 
    
    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "_result"); // 出错回复_error, 正常回复_result
    p += AMFAny::EncodeNumber(p, 1.0); // 固定为1
    *p++ = kAMFObject;
    p += AMFAny::EncodeNamedString(p, "fmsVer", "FMS/3,0,1,123");  // 版本号  
    p += AMFAny::EncodeNamedNumber(p, "capabilities", 31);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;
    *p++ = kAMFObject; 
    p += AMFAny::EncodeNamedString(p, "level", "status");
    p += AMFAny::EncodeNamedString(p, "code", "NetConnection.Connect.Success"); // 连接成功
    p += AMFAny::EncodeNamedString(p, "description", "Connection succeeded.");
    p += AMFAny::EncodeNamedNumber(p, "objectEncoding", amf3 ? 3.0 : 0);
    *p++ = 0x00;
    *p++ = 0x00;
    *p++ = 0x09;
    // 设置包长度
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "connect result msg_len:" << header->msg_len << " host:" << connection_->PeerAddr().ToIpPort();
    
    // 加入到待编码的队列中
    PushOutQueue(std::move(packet));
}

/// @brief 发送给客户端通知，创建了一个NetStream,客户端收到发起play命令
void RtmpContext::SendCreateStream()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "createStream");
    p += AMFAny::EncodeNumber(p, 4.0);
    *p++ = kAMFNull;

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "send create stream msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}

void RtmpContext::HandleCreateStream(AMFObject &obj)
{
    auto tran_id = obj.Property(1)->Number();

    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;
    header->msg_sid = 0;
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; 
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    p += AMFAny::EncodeString(p, "_result");
    p += AMFAny::EncodeNumber(p, tran_id);
    *p++ = kAMFNull;
    
    p += AMFAny::EncodeNumber(p, kRtmpMsID1);

    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "create stream result msg_len:" << header->msg_len << " to host:" << connection_->PeerAddr().ToIpPort();
    PushOutQueue(std::move(packet));
}

/// @brief NetStream的csid固定为3，ms_id固定为1.类型是20。服务端收到play命令，执行play操作，发送onstatus通知
/// @param level 
/// @param code 
/// @param description 
void RtmpContext::SendStatus(const std::string &level, const std::string &code, const std::string &description)
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;    // 使用AMF的初始化，固定为3
    header->msg_sid = 1;        // 固定值
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; // AMF0，消息使用AMF序列化
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    // 设置消息体
    p += AMFAny::EncodeString(p, "onStatus");    // command Name
    p += AMFAny::EncodeNumber(p, 0);  // Transaction ID 固定的0
    *p++ = kAMFNull;
    *p++ = kAMFObject;
    // command object
    p += AMFAny::EncodeNamedString(p, "level", level);    
    p += AMFAny::EncodeNamedString(p, "code", code);    
    p += AMFAny::EncodeNamedString(p, "description", description); 
 
    *p++ = 0x00;
    *p++ = 0x00;        // AMF
    *p++ = 0x09;

    // 设置包长度
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "send status level:" << level << "code:" << code  << "description:" << description << " host:" << connection_->PeerAddr().ToIpPort();
    
    // 加入到待编码的队列中
    PushOutQueue(std::move(packet)); 
}

void RtmpContext::SendPlay()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;    // 使用AMF的初始化，固定为3
    header->msg_sid = 1;        // netStream的命令固定为1
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; // AMF0，消息使用AMF序列化
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    // 设置消息体
    p += AMFAny::EncodeString(p, "play");    // command Name
    p += AMFAny::EncodeNumber(p, 0);  // Transaction ID 固定的1
    *p++ = kAMFNull;    // 空值
    p += AMFAny::EncodeString(p, name_);
    p += AMFAny::EncodeNumber(p, -1000.0);  
    // 设置包长度
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "send play name:" << name_ << "msg_len = " << header->msg_len << " host:" << connection_->PeerAddr().ToIpPort();
    
    // 加入到待编码的队列中
    PushOutQueue(std::move(packet));
}

/// @brief 服务器处理
/// @param obj 
void RtmpContext::HandlePlay(AMFObject &obj)
{
    // 1. 收到客户端的play，进行rtmp的url的解析
    auto tran_id = obj.Property(1)->Number();
    name_ = obj.Property(3)->String();
    ParseNameAndTcUrl();        // tcUrl在handleConnect的时候就解析了

    RTMP_TRACE << "recv play session_name:" << session_name_ << " param:" << param_ << " host:" << connection_->PeerAddr().ToIpPort();

    is_player_ = true; // 是播放的客户端
    // 2. 解析成功之后，发送streamBegin标志，通知客户端开始播放流
    SendUserCtrlMessage(kRtmpEventTypeStreamBegin, 1, 0);
        // 发送status
    SendStatus("status", "NetStream.Play.Start", "Start playing");

    if(rtmp_handler_)   // 如果业务层存在， 告诉业务层
    {
        rtmp_handler_->OnPlay(connection_, session_name_, param_); // 业务层一般创建用户，归纳到推流session中
    }
}

void RtmpContext::ParseNameAndTcUrl()
{
    auto pos = app_.find_first_of("/");
    if(pos!=std::string::npos)
    {
        app_ = app_.substr(pos+1);
    }

    param_.clear();
    pos = name_.find_first_of("?");
    if(pos != std::string::npos)
    {
        param_ = name_.substr(pos+1);
        name_ = name_.substr(0,pos);
    }

    std::string domain;

    std::vector<std::string> list = base::StringUtils::SplitString(tc_url_,"/");
    if(list.size()==5)//rmtp://ip/domain:port/app
    {
        domain = list[3];
        app_ = list[4];
    }
    else if(list.size() == 4) //rmtp://domain:port/app
    {
        domain = list[2];
        app_ = list[3];
    }

    auto p = domain.find_first_of(":");
    if(p!=std::string::npos)
    {
        domain = domain.substr(0,p);
    }

    // TODO: 收到域名直接用，如果是ip就要用Dns转成域名
    session_name_.clear();
    session_name_ += domain;
    session_name_ += "/";
    session_name_ += app_;
    session_name_ += "/";
    session_name_ += name_;

    RTMP_TRACE << "session_name:" << session_name_ 
            << " param:" << param_ 
            << " host:" << connection_->PeerAddr().ToIpPort();
}
/// @brief  客户端发送命令，发布一个有名字的流到服务器，其他客户端可以用名字进行拉流
void RtmpContext::SendPublish()
{
    PacketPtr packet = Packet::NewPacket(1024);
    RtmpMsgHeaderPtr header = std::make_shared<RtmpMsgHeader>();
    header->cs_id = kRtmpCSIDAMFIni;    // 使用AMF的初始化，固定为3
    header->msg_sid = 1;        // netStream的命令固定为1
    header->msg_len = 0;
    header->msg_type = kRtmpMsgTypeAMFMessage; // AMF0，消息使用AMF序列化
    packet->SetExt(header);

    char *body = packet->Data();
    char *p = body;

    // 设置消息体
    p += AMFAny::EncodeString(p, "publish");    // command Name
    p += AMFAny::EncodeNumber(p, 5);  
    *p++ = kAMFNull;    // 空值
    p += AMFAny::EncodeString(p, name_);
    p += AMFAny::EncodeString(p, "live"); // 指定 类型，这里是直播
    // 设置包长度
    header->msg_len = p - body;
    packet->SetPacketSize(header->msg_len);
    RTMP_TRACE << "send publish name:" << name_ << "msg_len = " << header->msg_len << " host:" << connection_->PeerAddr().ToIpPort();
    
    // 加入到待编码的队列中
    PushOutQueue(std::move(packet));
}

/// @brief 服务端处理publish命令，解析参数，拿到tran id回复
/// @param obj 
void RtmpContext::HandlePublish(AMFObject &obj)
{
    auto tran_id = obj.Property(1)->Number();
    name_ = obj.Property(3)->String();
    ParseNameAndTcUrl(); // fixbug:这里没有解析Name，导致后面的session_name拿不到

    RTMP_TRACE << "recv publish session_name:" << session_name_
                << " param :" << param_
                << " host:" << connection_->PeerAddr().ToIpPort();

    is_player_ = false;// 已经开始推流就不是拉流的了
    // 通知客户端开始推流
    SendStatus("status", "NetStream.Publish.Start", "Start publishing");

    if(rtmp_handler_)
    {
        // 通知业务层，以后所有的播放都是加入到session_name中
        rtmp_handler_->OnPublish(connection_, session_name_, param_);
    }
}


/// @brief 
/// @param obj 
void RtmpContext::HandleResult(AMFObject &obj)
{
    auto id = obj.Property(1)->Number();
    RTMP_TRACE << "recv result id: "<< id << " host:" << connection_->PeerAddr().ToIpPort();
    if(id == 1) // connect
    {
        // id = 1是connect的结果，之后就要进行创建NetStream
        SendCreateStream();
    }
    else if(id == 4) // NetStream结束
    {
        if(is_player_)// 是播放的客户端，就发送play请求
        {
            SendPlay(); 
        }
        else
        {
            SendPublish(); // 不是播放的，就是发布的
        }
    }
    else if(id == 5) // publish发送的时候是5
    {
        if(rtmp_handler_)
        {
            // 准备好了，可以往conection写数据了
            rtmp_handler_->OnPublishPrepare(connection_);
        }
    }
}

void RtmpContext::HandleError(AMFObject & obj)
{
    // 拿到描述
    const std::string &description = obj.Property(3)->Object()->Property("description")->String();
    RTMP_TRACE << "recv error description: "<< description << " host:" << connection_->PeerAddr().ToIpPort();
    connection_->ForceClose();
}

/// @brief 在收到packet的时候，里面保存的类型是rtmpmsg的类型，需要转换成packetType
/// @param packet 
void RtmpContext::SetPacketType(PacketPtr & packet)
{
    if(packet->PacketType() == kRtmpMsgTypeAudio)
    {
        packet->SetPacketType(kPacketTypeAudio);
    }
    if(packet->PacketType() == kRtmpMsgTypeVideo)
    {
        packet->SetPacketType(kPacketTypeVideo);
    }
    if(packet->PacketType() == kRtmpMsgTypeAMFMeta)   
    {
        packet->SetPacketType(kPacketTypeMeta);
    }
    if(packet->PacketType() == kRtmpMsgTypeAMF3Meta) // metadata3
    {
        packet->SetPacketType(kPacketTypeMeta3);
    }
}

/// @brief 客户端拉流
/// @param url 
void RtmpContext::Play(const std::string &url)
{
    is_client_ = true;
    is_player_ = true;
    tc_url_ = url;
    ParseNameAndTcUrl();
}

/// @brief 客户端推流
/// @param url 
void RtmpContext::Publish(const std::string &url)
{
    is_client_ = true;
    is_player_ = false; // 推流
    tc_url_ = url;
    ParseNameAndTcUrl();
}


bool RtmpContext::BuildChunk(PacketPtr &&packet, uint32_t timestamp, bool fmt0)
{
    RtmpMsgHeaderPtr header = packet->Ext<RtmpMsgHeader>();
    if(header)
    {
        // out_sending_packets_.emplace_back(std::move(packet));
        RtmpMsgHeaderPtr &prev = out_message_headers_[header->cs_id];   // 取出前一个发送的消息头
        // 1. 不指定fm0  2.有前一个，才会进行后面的包时间计算  3. 当前时间戳大于前一个的就不是fmt0,  当前的msg_sid和前一个一样，肯定不是fmt0
        bool use_delta = !fmt0 && prev && timestamp >= prev->timestamp && header->msg_sid == prev->msg_sid;
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
        if(timestamp >= 0xFFFFFF)      // 有没有使用额外的timestamp
        {
            ts = 0xFFFFFF;
        }

        // 开始组装header数据
        if(fmt == kRtmpFmt0)
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_len); // msg_len 3字节
            p += BytesWriter::WriteUint8T(p, header->msg_type);
            
            memcpy(p, &header->msg_sid, 4);     // streamid传输也是小端存放，不需要转换
            p += 4;
            out_deltas_[header->cs_id] = 0;
        }
        else if(fmt == kRtmpFmt1)       // 没有streamid
        {
            p += BytesWriter::WriteUint24T(p, ts); // ts 3字节
            p += BytesWriter::WriteUint24T(p, header->msg_len); // msg_len 3字节
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
        out_sending_packets_.emplace_back(std::move(packet));
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

/// @brief 加入到等待发送的队列中，发送过程中还要进行rtmp包的build
/// @param packet 
void RtmpContext::PushOutQueue(PacketPtr &&packet)
{
    out_waiting_queue_.emplace_back(std::move(packet));
    Send();
}

