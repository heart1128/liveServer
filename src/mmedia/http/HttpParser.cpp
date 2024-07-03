#include "HttpParser.h"
#include "base/StringUtils.h"
#include "mmedia/base/MMediaLog.h"
#include <algorithm>

using namespace tmms::mm;
using namespace tmms::base;

static std::string CRLFCRLF = "\r\n\r\n";
static int32_t kHttpMaxBodySize = 64 * 1024;

namespace
{
    static std::string string_empty;
}

/// @brief 解析入口，状态机解析，每个状态对应不同函数调用
/// @param buf 
/// @return 返回当前解析的状态
HttpParserState HttpParser::Parse(MsgBuffer &buf)
{
    if(buf.ReadableBytes() == 0)
    {
        return state_;
    }

    switch (state_)
    {
    case kExpectHeaders:
    {
        // 请求头至少有4字节 method 
        if(buf.ReadableBytes() > CRLFCRLF.size())
        {
            // 查找\r\n\r\n
            auto *space = std::search(buf.Peek(), (const char*)buf.BeginWrite(), CRLFCRLF.data(), CRLFCRLF.data() + CRLFCRLF.size());
            if(space != (const char*)buf.BeginWrite()) // 找到的结束符不是数据结尾
            {
                auto size = space - buf.Peek();
                header_.assign(buf.Peek(), size);
                buf.Retrieve(size+4); 
                ParseHeaders(); // 解析头
                if(state_ == kExpectHttpComplete || state_ == kExpectError)
                {
                    return state_;
                }
            }
            else // 没找到就是异常了
            {
                if(buf.ReadableBytes() > kHttpMaxBodySize)
                {
                    reason_ = k400BadRequest;
                    state_ = kExpectError;
                    return state_;
                }
                return kExpectContinue;
            }
        }
        else // 不够4字节，继续接收数据
        {
            return kExpectContinue;
        }
        break;
    }
    case kExpectNormalBody:
    {
        ParseNormalBody(buf);
        break;
    }
    case kExpectStreamBody:
    {
        ParseStream(buf);
        break;
    }
    case kExpectChunkLen:   // 请求头发现是chunk方式，就进入这里找到chunksize
    {
        auto crlf = buf.FindCRLF();
        if(crlf)
        {
            // 请求头之后就是长度
            /**
             * HTTP/1.1 200 OK
                Content-Type: text/plain
                Transfer-Encoding: chunked 0D 0A
                // 块1
                1A   ----> 长度
                Hello, World! 0D 0A   -----> \r\n

                // 块2
                0 
                0D 0A
             * 
            */
            std::string len(buf.Peek(), crlf);
            char *end;
            current_chunk_length_ = std::strtol(len.c_str(), &end, 16); // 16进制转
            HTTP_DEBUG << " chunk len:" << current_chunk_length_;
            if(current_chunk_length_ > 1024 * 1024) // 太大了
            {
                HTTP_ERROR << "error chunk len.";
                state_ = kExpectError;
                reason_ = k400BadRequest;
            }

            buf.RetrieveUntil(crlf + 2);
            if(current_chunk_length_ == 0)
            {
                state_ = kExpectLastEmptyChunk; // 最后chunk发送结束，后面还有一行是\r\n
            }
            else
            {
                state_ = kExpectChunkBody;
            }
        }
        else // 找不到\r\n
        {
            // 并且数据很多，可能是一个非法的请求
            if(buf.ReadableBytes() > 32)
            {
                buf.RetrieveAll();
                reason_ = k400BadRequest;
                state_ = kExpectError;
                return state_;
            }
        }
        break;
    }
    case kExpectChunkBody:
    {
        ParseChunk(buf);
        if(state_ == kExpectChunkComplete)
        {
            return state_;
        }
        break;
    }
    case kExpectLastEmptyChunk: // 只有\r\n了
    {
        auto crlf = buf.FindCRLF();
        if(crlf)
        {
            buf.RetrieveUntil(crlf + 2);
            chunk_.reset();
            state_ = kExpectHttpComplete;
            break;

        }
    }
    default:
        break;
    }
    return state_;
}

const PacketPtr &HttpParser::Chunk() const
{
    return chunk_;
}

HttpStatusCode HttpParser::Reason() const
{
    return reason_;
}

/// @brief 清理所有状态
void HttpParser::ClearForNextHttp()
{
    state_ = kExpectHeaders;
    header_.clear();
    req_.reset();
    current_content_length_ = -1;
    chunk_.reset();
}

/// @brief 清理一下包
void HttpParser::ClearForNextChunk()
{
    // 是chunk，结束了等待下一个chunklen
    if(is_chunk_)
    {
        state_ = kExpectChunkLen;
        current_chunk_length_ = -1;
    }
    else
    {
        if(is_stream_) // 是流，直接进流body
        {
            state_ = kExpectStreamBody;
        }
        else    // 否则就是标准http包，已经解析结束了，等下一个header
        {
            state_ = kExpectHeaders;
            current_content_length_ = -1;
        }
    }
}


/// @brief 流数据，没有边界，累计长度保存
/// @param buf 
void HttpParser::ParseStream(MsgBuffer &buf)
{
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(kHttpMaxBodySize);
    }
    // 读取数据到chunk中
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);

    if(chunk_->Space() == 0)    // 流数据包就是解析kHttpMaxBodySize，就结束
    {
        state_ = kExpectHttpComplete;
    }

}

/// @brief http的标准body
/// @param buf 
void HttpParser::ParseNormalBody(MsgBuffer &buf)
{
    // chunk_保存数据
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(current_content_length_);
    }
    // 读取数据到chunk中
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);

    current_content_length_ -= size;
    if(current_content_length_ == 0)    // 解析完成，没解析完成还是这个状态，再进入函数
    {
        state_ = kExpectHttpComplete;
    }

}

void HttpParser::ParseChunk(MsgBuffer &buf)
{
    if(!chunk_)
    {
        chunk_ = Packet::NewPacket(current_chunk_length_);
    }
    // 读取数据到chunk中
    auto size = std::min((int)buf.ReadableBytes(), chunk_->Space());
    memcpy(chunk_->Data() + chunk_->PacketSize(), buf.Peek(), size);
    chunk_->UpdatePacketSize(size);
    buf.Retrieve(size);

    current_content_length_ -= size;
    if(current_chunk_length_ == 0 || chunk_->Space() == 0)    // 解析完成，没解析完成还是这个状态，再进入函数
    {
        state_ = kExpectChunkComplete;
    }
}

/// @brief 解析请求头键值对
void HttpParser::ParseHeaders()
{
    auto list = StringUtils::SplitString(header_, "\r\n");
    // 请求头都没有
    if(list.size() < 1)
    {
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return;
    }

    ProcessMethodLine(list[0]); // 解析请求命令行，放头进去解析

    for(auto &l : list)
    {
        auto pos = l.find_first_of(':'); // 找key:value
        if(pos != std::string::npos)
        {
            std::string k = l.substr(0, pos);
            std::string v = l.substr(pos + 1);

            HTTP_DEBUG << "parse header k:" << k << " v:" << v;
            req_->AddHeader(std::move(k), std::move(v));
        }
    }
    auto len = req_->GetHeader("content-length");
    if(!len.empty())
    {
        HTTP_TRACE << "content-length:" << len;
        try
        {
            // 转成long long
            current_content_length_ = std::stoull(len);
        }
        catch(...)
        {
            reason_ = k400BadRequest;
            state_ = kExpectError;
            return;
        }
        
        // 没有body，请求头解析完直接结束
        if(current_chunk_length_ == 0)
        {
            state_ = kExpectHttpComplete;
        }
        else
        {
            state_ = kExpectNormalBody;
        }
    }
    else  // 如果不带len。可能是chunk,可能是stream，也可能是响应包
    {
        // 1. 请求头格式-> transfer-encoding:chunked
        const std::string &chunk = req_->GetHeader("transfer-encoding");
        if(!chunk.empty() && chunk == "chunked")
        {
            is_chunk_ = true;
            req_->SetIsChunked(true);
            state_ = kExpectChunkLen;
        }
        else // 不是chunk的情况
        {
            if((!is_request_ && req_->GetStatusCode() != 200)    // 是响应，或者是不带内容的空请求，get、head、options
                || (is_request_ 
                && (req_->Method() == kGet || req_->Method() == kHead || req_->Method() == kOptions)))
            {
                current_content_length_ = 0;
                state_ = kExpectHttpComplete;
            }
            else        // stream
            {
                current_content_length_ = -1;
                is_stream_ = true;
                req_->SetIsStream(is_stream_);
                state_ = kExpectStreamBody;
            }
        }
    }
}

void HttpParser::ProcessMethodLine(const std::string &line)
{
    // method 空格 url 空格 http版本 \r\n   请求
    // httpversion  空格 code 空格 code desc \r\n 响应
    auto list = StringUtils::SplitString(line, " ");
    if(list.size() != 3)
    {
        reason_ = k400BadRequest;
        state_ = kExpectError;
        return;
    }
    std::string str = list[0]; // method
    // 转小写
    std::transform(str.begin(), str.end(), str.begin(), ::tolower);

    if(str[0] == 'h' && str[0] == 't' && str[2] == 't' && str[3] == 'p') // http1.1 or http 1.0 等都是响应格式
    {
        is_request_ = false;
    }
    else
    {
        is_request_ = true;
    }

    if(req_)
    {
        req_.reset();
    }

    req_ = std::make_shared<HttpRequest>(is_request_);

    if(is_request_)     // request
    {
        req_->SetMethod(std::move(list[0]));
        const std::string &path = list[1]; // url
        auto pos = path.find_first_of("?"); // 是不是带了query  
        if(pos != std::string::npos)
        {
            req_->SetPath(path.substr(0, pos));
            req_->SetQuery(path.substr(pos + 1));
        }
        else
        {
            req_->SetPath(path.substr(0, pos));
        }
        req_->SetVersion(list[2]);

        HTTP_DEBUG << "http method:" << list[0] << " path:" << req_->Path() 
                    << " query:" << req_->Query() << " version:" << list[2];
    }
    else        // response
    {
        req_->SetVersion(list[0]);
        req_->SetStatusCode(std::atoi(list[1].c_str()));
        HTTP_DEBUG << "http code:" << list[1]
                    << " version:" << list[0];
    }
}
