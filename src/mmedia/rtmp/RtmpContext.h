/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 14:59:42
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-12 20:49:54
 */
#pragma once

#include "network/net/TcpConnection.h"
#include "RtmpHandShake.h"
#include "RtmpHandler.h"
#include "RtmpHeader.h"
#include <cstdint>
#include <unordered_map>

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;

        enum RtmpContextState      
        {
            kRtmpHandShake = 0, // 收到S0S1S2之后转换成下一个状态
            kRtmpWatingDone = 1,// 发送C2状态下一个
            kRtmpMessage = 2,   // 握手接受，数据传输
        };

        class RtmpContext   // rtmp上下文
        {
        public:
            RtmpContext(const TcpConnectionPtr &con, RtmpHandler *handler, bool client = false);
            ~RtmpContext() = default;

        public:
            int32_t Parse(MsgBuffer &buf);
            void OnWriteComplete();
            void StarHandShake();

            int32_t ParseMessage(MsgBuffer &buf);
            void MessageComplete(PacketPtr && data);

        private:
            RtmpHandShake handshake_;           // rtmp握手
            int32_t state_ {kRtmpHandShake};
            TcpConnectionPtr connection_;
            RtmpHandler *rtmp_handler_{nullptr};
            std::unordered_map<uint32_t, RtmpMsgHeaderPtr> in_message_headers_; // csid作为key, 包头作为value
            std::unordered_map<uint32_t, PacketPtr> in_packets_;                // csid
            std::unordered_map<uint32_t, uint32_t> in_deltas_;                  // csid
            std::unordered_map<uint32_t, bool> in_ext_;                         // 额外的timestamp

            int32_t in_chunk_size_{128};
        };

        using RtmpContextPtr = std::shared_ptr<RtmpContext>;
        
    } // namespace mm
    
} // namespace tmms
