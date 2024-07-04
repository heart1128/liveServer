/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 14:59:42
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 15:41:44
 */
#pragma once

#include "network/net/TcpConnection.h"
#include "RtmpHandShake.h"
#include "RtmpHandler.h"
#include "RtmpHeader.h"
#include "amf/AMFObject.h"
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

        // 枚举定义不同的 用户RTMP 事件类型
        enum RtmpEventType
        {
            kRtmpEventTypeStreamBegin = 0,      // 流开始
            kRtmpEventTypeStreamEOF,            // 流结束
            kRtmpEventTypeStreamDry,            // 流变为空（无数据）
            kRtmpEventTypeSetBufferLength,      // 设置缓冲长度
            kRtmpEventTypeStreamsRecorded,      // 录制的流
            kRtmpEventTypePingRequest,          // Ping 请求
            kRtmpEventTypePingResponse          // Ping 响应
        };


        using CommandFunc = std::function<void (AMFObject &obj)>;
        class RtmpContext   // rtmp上下文
        {
        public:
            RtmpContext(const TcpConnectionPtr &con, RtmpHandler *handler, bool client = false);
            ~RtmpContext() = default;

        public:
            int32_t Parse(MsgBuffer &buf);
            void OnWriteComplete();
            void StartHandShake();
            // 接收数据解析
            int32_t ParseMessage(MsgBuffer &buf);
            void MessageComplete(PacketPtr && data);
            // 发送数据包
            bool BuildChunk(const PacketPtr &packet, uint32_t timestamp = 0, bool fmt0 = false);
            void Send();
            bool Ready() const;
            // 控制消息
                // 发送 rtmp控制消息
            void SendSetChunkSize();        // 块大小
            void SendAckWindowSize();      // 确认窗口大小
            void SendSetPeerBandwidth();    // 带宽，和窗口大小值是一样的
            void SendBytesRecv(); // 发送消息
                // 用户控制消息
            void SendUserCtrlMessage(short nType, uint32_t value1, uint32_t value2); // 发送用户控制消息
                // 接收处理
            void HandleChunkSize(PacketPtr &packet);
            void HandleAckWindowSize(PacketPtr &packet);
            void HandleUserMessage(PacketPtr &packet);
            // AMF相关
            void HandleAmfCommand(PacketPtr &data, bool amf3 = false);
            // NetConnection相关 服务端和客户端之间进行网络连接的高级表现形式
            void SendConnect();
            void HandleConnect(AMFObject &obj);

            // NetStream相关， 传输音视频的命令消息
                // 直播，只实现了play和publish
            void SendCreateStream();
            void HandleCreateStream(AMFObject &obj);
            /// @brief 服务器通过发送onStatus给客户端通知网络流的状态更新
            /// @param level  info object，至少有三个参数
            /// @param code 
            /// @param description 
            void SendStatus(const std::string &level, const std::string &code, const std::string &description);
    
            void SendPlay(); // 客户端使用
            void HandlePlay(AMFObject &obj); // 服务端处理
            void ParseNameAndTcUrl(); // 解析名称和推流地址
            void SendPublish();  // 客户端发送命令，发布一个有名字的流到服务器，其他客户端可以用名字进行拉流
            void HandlePublish(AMFObject &obj);
                // 对端收到Netconnection命令消息之后，进行回复，用_result或者_error回复
            void HandleResult(AMFObject &obj);
            void HandleError(AMFObject &obj);
            void SetPacketType(PacketPtr &packet);
            // 客户端，拉流和推流
            void Play(const std::string &url);
            void Publish(const std::string &url);

        private:
            bool BuildChunk(PacketPtr &&packet, uint32_t timestamp = 0, bool fmt0 = false);
            void CheckAndSend();
            void PushOutQueue(PacketPtr &&packet);
            ///////// 握手加接收解析头
            RtmpHandShake handshake_;           // rtmp握手
            int32_t state_ {kRtmpHandShake};
            TcpConnectionPtr connection_;
            RtmpHandler *rtmp_handler_{nullptr};
            std::unordered_map<uint32_t, RtmpMsgHeaderPtr> in_message_headers_; // csid作为key, 包头作为value
            std::unordered_map<uint32_t, PacketPtr> in_packets_;                // csid
            std::unordered_map<uint32_t, uint32_t> in_deltas_;                  // fmt=3，delta和之前的一样，所以要保存
            std::unordered_map<uint32_t, bool> in_ext_;                         // 额外的timestamp
            int32_t in_chunk_size_{128}; 
            /////////   发送数据
            char out_buffer_[4096];
            char *out_current_{nullptr};                // 缓存out_buffer当前读的指针
            std::unordered_map<uint32_t, uint32_t> out_deltas_;     // 发送的deltas
            std::unordered_map<uint32_t, RtmpMsgHeaderPtr> out_message_headers_;
            int32_t out_chunk_size_{4096};              // 最大包的大小
            std::list<PacketPtr> out_waiting_queue_;    // 等待build的处理，是存放网络接收的packet
            std::list<BufferNodePtr> sending_bufs_;     // buildChunk的数据写在这里保存等待发送,tcp发送的就是这个
            std::list<PacketPtr> out_sending_packets_;  // 发送的包， 正在发送的包，延长生命周期
            bool sending_{false};                       // 是否在发送
            /////////  控制消息
            int32_t ack_size_{2500000};     // 确认窗口
            int32_t in_bytes_{0};           // 接收到的数据
            int32_t last_left_{0};          // 剩下的数据
            ///////// netConnect
            // 例子  rtmps://server-address:port/app-name/stream-name?key1=value1&key2=value2
            std::string app_;   // 推流点 app-name
            std::string tc_url_; // 推流url  rtmps://server-address:port/app-name/stream-name。
            std::string name_;  // 推流名 stream-name，推流名称，用于唯一标识正在推送的流。
            std::string session_name_; // server-address:port/app-name/stream-name?key1=value1&key2=value2
            std::string param_;  // 推流参数
            bool is_player_{false}; // 是不是一个播放客户端

            // 保存所有命令的回调（connect之类的）
            std::unordered_map<std::string, CommandFunc> commands_;

            // 客户端
            bool is_client_{false}; // 标识客户端
        };

        using RtmpContextPtr = std::shared_ptr<RtmpContext>;
        
    } // namespace mm
    
} // namespace tmms
