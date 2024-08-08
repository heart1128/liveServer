#pragma once

#include "mmedia/base/Packet.h"
#include <cstdint>
#include <string>
#include <openssl/ssl.h>
#include <openssl/hmac.h>

namespace tmms
{
    namespace mm
    {
        
        const uint32_t kStunMagicCookie = 0x2112A442; // sutn的固定值
        enum StunMessageType 
        {
            kStunMsgUnknow                    = 0x0000,
            kStunMsgBindingRequest            = 0x0001,
            kStunMsgBindingResponse           = 0x0101,
            kStunMsgBindingErrorResponse      = 0x0111,
            kStunMsgSharedSecretRequest       = 0x0002,
            kStunMsgSharedSecretResponse      = 0x0102,
            kStunMsgSharedSecretErrorResponse = 0x0112,
        };

        enum StunAttributeType
        {
            // https://tools.ietf.org/html/rfc3489#section-11.2
            kStunAttrMappedAddress     = 0x0001,
            kStunAttrResponseAddress   = 0x0002,
            kStunAttrChangeRequest     = 0x0003,
            kStunAttrSourceAddress     = 0x0004,
            kStunAttrChangedAddress    = 0x0005,
            kStunAttrUsername          = 0x0006,
            kStunAttrPassword          = 0x0007,
            kStunAttrMessageIntegrity  = 0x0008,
            kStunAttrErrorCode         = 0x0009,
            kStunAttrUnknownAttributes = 0x000A,
            kStunAttrReflectedFrom     = 0x000B,
            // https://tools.ietf.org/html/rfc5389#section-18.2
            kStunAttrRealm             = 0x0014,
            kStunAttrNonce             = 0x0015,
            kStunAttrXorMappedAddress  = 0x0020,
            kStunAttrSoftware          = 0x8022,
            kStunAttrAlternateServer   = 0x8023,
            kStunAttrFingerprint       = 0x8028,
            kStunAttrPriority          = 0x0024,
            kStunAttrUseCandidate      = 0x0025,
            kStunAttrIceControlled     = 0x8029,
            kStunAttrIceControlling    = 0x802A,
        };

        class Stun
        {
        public:
            Stun() = default;
            ~Stun() = default;
        
        public:
            bool Decode(const char* data,uint32_t size);
            PacketPtr Encode();
            std::string LocalUFrag() ;
            void SetPassword(const std::string &pwd);
            void SetMappedAddr(uint32_t addr);
            void SetMappedPort(uint16_t port);
            void SetMessageType(StunMessageType type);
            size_t CalcHmac(char *buf, const char *data, size_t bytes);

        private:
            StunMessageType type_{kStunMsgUnknow};
            int32_t stun_length_{0};
            std::string transcation_id_;
            std::string user_name_;
            std::string password_;
            uint32_t mapped_addr_{0}; // 映射的IP地址和端口
            uint16_t mapped_port_{0};
        };
        
        
    } // namespace mm
    
}