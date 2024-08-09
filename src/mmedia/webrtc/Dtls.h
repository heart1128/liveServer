#pragma once

#include <cstdint>
#include <string>
#include <openssl/ssl.h>
#include <openssl/x509.h>
#include <openssl/bio.h>
#include "DtlsHandler.h"
#include "DtlsCerts.h"

namespace tmms
{
    namespace mm
    {
        class Dtls
        {
        public:
            Dtls(DtlsHandler *handler);
            ~Dtls();
        
            bool Init();
            void OnRecv(const char *data,uint32_t size);
            const std::string &Fingerprint() const;
            void SetDone();
            void SetClient(bool client);
            const std::string &SendKey();
            const std::string &RecvKey();
        
        private:
            bool InitSSLContext();
            bool InitSSL();
            static int SSLVerify(int preverify_ok, X509_STORE_CTX *ctx);
            static void SSLInfo(const SSL* ssl, int where, int ret);
            void NeedPost();
            void GetSrtpKey();


            // DTLS和SSL/TSL差不多，对UDP做出了适配，依赖SSL
            SSL_CTX * ssl_context_{nullptr};
            DtlsCerts dtls_cert_;
            bool is_client_{false};
            bool is_done_{false};
            SSL * ssl_{nullptr};
            BIO * bio_read_{nullptr}; // bio是ssl的抽象IO接口，用于处理IO
            BIO * bio_write_{nullptr};
            char buffer_[65535];
            DtlsHandler * handler_{nullptr};
            std::string send_key_;
            std::string recv_key_;
        };
        
        
    }
}