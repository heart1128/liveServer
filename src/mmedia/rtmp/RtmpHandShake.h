/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 20:18:59
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-10 09:28:08
 * @FilePath: /tmms/src/mmedia/rtmp/RtmpHandShake.h
 * @Description:  learn 
 */

#pragma once

#include <string>
#include "network/net/TcpConnection.h"
#include <memory>
#include <openssl/sha.h>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        using namespace network;

        const int kRtmpHandShakePacketSize = 1536;      // C2,S2的大小 , + 1就是C1S1
        enum RtmpHandShakeState
        {
            kHandShakeInit,
            // client
            kHandShakePostC0C1, // 发送C0C1
            kHandShakeWaitS0S1, // 等待S0S1
            kHandShakePostC2,
            kHandShakeWaitS2,
            kHandShakeDoning,   // 结束中，因为发送完S1可以直接发送S2，客户端可能就处于等待接受处理中
            // server
            kHandShakeWaitC0C1,
            kHandShakePostS0S1,
            kHandShakePostS2,
            kHandShakeWaitC2,
            kHandShakeDone,
        };

        class RtmpHandShake
        {
        public:
            // 因为要区分服务端和客户端 
            RtmpHandShake(const TcpConnectionPtr &conn, bool client);
            ~RtmpHandShake() = default;
        
        public:
            void Start();

            int32_t HandShake(MsgBuffer &buf);
            void WriteComplete();
            
        
        private:
            uint8_t GenRandom();

            void CreateC1S1();
            int32_t CheckC1S1(const char *data, int bytes);
            void SendC1S1();

            void CreateC2S2(const char *data, int bytes, int offset);
            bool CheckC2S2(const char *data, int bytes);
            void SendC2S2();

            TcpConnectionPtr connection_;
            bool is_client_{false};             //默认服务端
            bool is_complex_handshake_{true}; // 默认复杂握手，digest失败或者明确指定才使用简单的握手
            uint8_t digest_[SHA256_DIGEST_LENGTH];  // digest-data
            uint8_t C1S1_[kRtmpHandShakePacketSize + 1]; // c1 s1公用，不是同一端的1537字节固定
            uint8_t C2S2_[kRtmpHandShakePacketSize];

            int32_t state_{kHandShakeInit};
        };
    
        
    } // namespace mm
    
} // namespace tmms
