#pragma once

#include "HttpParser.h"
#include "HttpRequest.h"
#include "mmedia/base/Packet.h"
#include "network/net/EventLoop.h"
#include "HttpHandler.h"
#include "HttpParser.h"


namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;

        enum HttpContextPostState  // 发送的状态
        {
            kHttpContextPostInit,
            kHttpContextPostHttp,
            kHttpContextPostHttpHeader,
            kHttpContextPostHttpBody,
            kHttpContextPostHttpStreamHeader,
            kHttpContextPostHttpStreamChunk,
            kHttpContextPostChunkHeader, 
            kHttpContextPostChunkLen, // chunk每次都是先发len ，然后发chunk
            kHttpContextPostChunkBody,
            kHttpContextPostChunkEOF
        };


        class HttpContext
        {
        public:
            HttpContext(const TcpConnectionPtr &conn, HttpHandler *handler);
            ~HttpContext() = default;

            int32_t Parse(MsgBuffer &buf);
            bool PostRequest(const std::string &header_and_body); 
            bool PostRequest(const std::string &header, PacketPtr &packet);
            bool PostRequest(HttpRequestPtr &request);
            bool PostChunkHeader(const std::string &header);
            void PostChunk(PacketPtr &chunk);
            void PostEofChunk();
            bool PostStreamHeader(const std::string &header);
            bool PostStreamChunk(PacketPtr &packet);
            void WriteComplete(const TcpConnectionPtr &);
        
        private:
            TcpConnectionPtr connection_;
            HttpParser http_parser_;
            std::string header_;
            PacketPtr out_packet_;
            HttpContextPostState post_state_{kHttpContextPostInit};
            bool header_sent_; 
            HttpHandler *handler_{nullptr}; // 业务层可以通过这个句柄拿到回调
        };

        using HttpContextPtr = std::shared_ptr<HttpContext>;
        
    } // namespace mm
    
} // namespace tmms
