#pragma once

#include "network/net/TcpConnection.h"
#include "mmedia/base/Packet.h"
#include "mmedia/base/MMediaHandler.h"
#include "mmedia/base/MMediaHandler.h"
#include <string>
#include <list>
#include <memory>

/**
 * flv格式是容器封装格式，适用点播系统，文件小传输快
 * 
 * conetex主要是发送封装flv，然后解析到，或者发送之后调用上层回调
 * 
*/

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;

        class FlvContext
        {
        public:
            FlvContext(const TcpConnectionPtr& conn, MMediaHandler *handler);
            ~FlvContext() = default;
        
        public:
            // 发送http头
            void SendHttpHeader(bool has_video, bool has_audio);
            void WriteFlvHeader(bool has_video, bool has_audio);
            bool BuildFlvFrame(PacketPtr &pkt, uint32_t timestamp);
            void Send();
            void WriterComplete(const TcpConnectionPtr &conn);
            bool Ready()const;
        private:
            char GetRtmpPacketType(PacketPtr & pkt);
            std::list<BufferNodePtr> bufs_;     // 要发送的列表，所有东西都封装成BufferNode
            std::list<PacketPtr> out_packets_;  // 存放外部发送进来的包数据，构建flv frame
            TcpConnectionPtr connection_;
            uint32_t previous_size_{0};     // flv中的pre_size
            std::string http_header_;
            char out_buffer_[512];      // 中间缓存的buffer
            char *current_{nullptr};       // 指向out_buffer读写
            bool sending_{false};
            MMediaHandler *handler_{nullptr};
        };

        using FlvContextPtr = std::shared_ptr<FlvContext>;    
    } // namespace 
    
} // namespace tmms
