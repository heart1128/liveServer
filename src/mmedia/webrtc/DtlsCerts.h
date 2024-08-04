/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 17:43:04
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 17:59:46
 * @FilePath: /liveServer/src/mmedia/webrtc/DtlsCerts.h
 * @Description:  learn 
 */
#pragma once

#include <cstdint>
#include <string>
#include <openssl/x509.h> // 证书用到
#include <openssl/ssl.h>
#include <openssl/bio.h>

namespace tmms
{
    namespace mm
    {
        class DtlsCerts
        {
        public:
            DtlsCerts() = default;
            ~DtlsCerts();

            bool Init();
            const std::string &Fingerprint() const;  // 签名摘要（指纹）
            EVP_PKEY *GetPrivateKey() const;  // 私钥
            X509 *GetCerts() const;
            uint32_t GenRandom();
        
        private:
            EVP_PKEY *dtls_pkey_{nullptr};   // 私钥
            X509 * dtls_certs_{nullptr};    // 证书
            std::string fingerprint_;
        };
        
    } // namespace mm
    
} // namespace tmms
