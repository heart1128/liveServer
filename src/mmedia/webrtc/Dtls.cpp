/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-08 21:14:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-18 15:24:17
 * @FilePath: /liveServer/src/mmedia/webrtc/Dtls.cpp
 * @Description:  learn 
 */
#include "Dtls.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;
Dtls::Dtls(DtlsHandler *handler)
:handler_(handler)
{
}
Dtls::~Dtls()
{
    if(ssl_context_)
    {
        SSL_CTX_free(ssl_context_);
        ssl_context_ = nullptr;
    }
    if(ssl_)
    {
        SSL_free(ssl_);
        ssl_ = nullptr;
    }
    if(bio_read_)
    {
        BIO_free(bio_read_);
        bio_read_ = nullptr;
    }
    if(bio_write_)
    {
        BIO_free(bio_write_);
        bio_write_ = nullptr;
    }
}

bool Dtls::Init()
{
    // 初始化秘钥，证书生成
    auto ret = dtls_cert_.Init();
    if(!ret)
    {
        return false;
    }
    ret = InitSSLContext();
    if(!ret)
    {
        return false;
    }
    ret = InitSSL(); // 被动的，等待SSL数据
    return true;
}

/**
 * @description: 通过SSL收到数据
 * @param {char} *data
 * @param {uint32_t} size
 * @return {*}
 */
void Dtls::OnRecv(const char *data, uint32_t size)
{
    // 每次写都要重置内容
    BIO_reset(bio_read_);
    BIO_reset(bio_write_);

    BIO_write(bio_read_, data, size);
    SSL_do_handshake(ssl_);  // ssl握手，openssl实现好了

    // 收到了数据(不一定是握手)，接下来服务端发送数据
    NeedPost();

    // Dtls握手完成，得到一个key（客户端和服务端握手得到的），用作rtp的数据传输加密
    // rtp升级成了srtp，数据安全的
    if(is_done_)
    {
        GetSrtpKey();
        if(handler_)
        {
            handler_->OnDtlsHandshakeDone(this);
        }
        // 加密结束了就不需要处理了
        return;
    }
    SSL_read(ssl_, buffer_, 65535);
}

const std::string &Dtls::Fingerprint() const
{
    return dtls_cert_.Fingerprint();
}


/**
 * @description: 在Dtls成功握手加密结束后，设置这个标志
 * @return {*}
 */
void Dtls::SetDone()
{
    is_done_ = true;
}

void Dtls::SetClient(bool client)
{
    is_client_ = client;
}

const std::string &Dtls::SendKey()
{
    return send_key_;
}

const std::string &Dtls::RecvKey()
{
    return recv_key_;
}

bool Dtls::InitSSLContext()
{
    ssl_context_ = SSL_CTX_new(DTLS_method());
    SSL_CTX_use_certificate(ssl_context_, dtls_cert_.GetCerts()); // 设置自验证证书
    auto ret = SSL_CTX_use_PrivateKey(ssl_context_, dtls_cert_.GetPrivateKey()); // 设置服务端私钥
    if(!ret)
    {
        WEBRTC_ERROR << "SSL_CTX_use_PrivateKey failed.";
        return false;
    }
    ret = SSL_CTX_check_private_key(ssl_context_); // 检查私钥
    if(!ret)
    {
        WEBRTC_ERROR << "SSL_CTX_check_private_key failed.";
        return false;
    }
    // 设置加密套件（设置为支持所有套件）
    // 套件就是在不同阶段使用不同的加密算法，就属于不同套件
    // 套件里面有很多加密算法
    SSL_CTX_set_cipher_list(ssl_context_, "ALL");
    // 设置验证
    // 设置 SSL/TLS 上下文（SSL_CTX）的证书验证参数。这个函数允许你指定在 SSL 握手过程中对对方证书进行验证的行为，以及如何处理验证结果。
    SSL_CTX_set_verify(ssl_context_, SSL_VERIFY_PEER | SSL_VERIFY_CLIENT_ONCE, Dtls::SSLVerify);
    // 设置infocallback，
    // 该函数将在 SSL/TLS 握手过程中的特定事件发生时被调用。这个回调函数可以用于日志记录、调试或其他需要跟踪 SSL/TLS 连接状态的目的
    SSL_CTX_set_info_callback(ssl_context_, Dtls::SSLInfo);
    // 设置验证级别
    // 超过4层证书不能识别
    SSL_CTX_set_verify_depth(ssl_context_, 4);
    // 预读数据
    // 在非阻塞IO中，预读就是把数据全部读完，减少IO
    SSL_CTX_set_read_ahead(ssl_context_, 1); 
    // 设置srtp的扩展
    // 配置 SSL/TLS 上下文（SSL_CTX）以使用 SRTP（安全实时传输协议）。
    // 定义了加密算法和密钥管理协议，例如 SRTP_AES128_CM_SHA1_80
    SSL_CTX_set_tlsext_use_srtp(ssl_context_, "SRTP_AES128_CM_SHA1_80");
    return true;
}   

bool Dtls::InitSSL()
{
    ssl_ = SSL_new(ssl_context_); // 创建ssl到上下文
    if(!ssl_)
    {
        WEBRTC_ERROR << "SSL_new failed.";
        return false;
    }
    // 允许应用程序在 SSL 对象上存储额外的信息，这些信息可以在后续的操作中通过 SSL_get_ex_data 函数检索。
    SSL_set_ex_data(ssl_, 0, static_cast<void*>(this));
    // 创建读写的bio
    bio_read_ = BIO_new(BIO_s_mem());
    bio_write_ = BIO_new(BIO_s_mem());
    SSL_set_bio(ssl_, bio_read_, bio_write_);
    
    // 设置MTU
    SSL_set_mtu(ssl_, 1350);
    SSL_set_accept_state(ssl_); // 现在是服务端，设置accept状态
    return true;
}

int Dtls::SSLVerify(int preverify_ok, X509_STORE_CTX *ctx)
{
    return 1;
}

/**
 * @description: 处理SSL握手过程中的信息，是一个回调
 * @param {SSL} *ssl
 * @param {int} where
 * @param {int} ret
 * @return {*}
 */
void Dtls::SSLInfo(const SSL *ssl, int where, int ret)
{
    Dtls *dtls = static_cast<Dtls*>(SSL_get_ex_data(ssl, 0));
    int w = where & ~SSL_ST_MASK;
    
    if (w & SSL_ST_CONNECT)  // 是connect状态说明是客户端
    {
        dtls->SetClient(true);
    } 
    else if (w & SSL_ST_ACCEPT)  // accept是服务端
    {
        dtls->SetClient(false);
    } 
    else 
    {
        dtls->SetClient(false);
    }
    
    if (where & SSL_CB_HANDSHAKE_DONE)  // 握手结束设置标记
    {
        dtls->SetDone();
        WEBRTC_TRACE << "dtls handshake done.";
    }
}

/**
 * @description: 通过dtls发送数据
 * @return {*}
 */
void Dtls::NeedPost()
{
    // 有没有读完
    if(BIO_eof(bio_write_))
    {
        return;
    }

    char *data = nullptr;
    // 拿数据
    auto read = BIO_get_mem_data(bio_write_, &data);
    if(read <= 0)
    {
        return;
    }

    if(handler_)
    {
        handler_->OnDtlsSend(data, read, this);
    }
    BIO_reset(bio_write_);
}

/**
 * @description: 从Dtls握手中生成的秘钥，通过接口取出
 * @return {*}
 */
void Dtls::GetSrtpKey()
{
    int32_t srtp_key_len = 16;  
    int32_t srtp_salt_len = 14; 

    unsigned char material[30*2];
    static std::string label = "EXTRACTOR-dtls_srtp"; // 固定的
    // 从 SSL/TLS 中导出秘钥材料(上面label设置了srtp，所以导出srtp的)
    auto ret = SSL_export_keying_material(ssl_, material, sizeof(material), 
                                label.c_str(), label.size(), NULL, 0, 0);
    
    if(ret <= 0)
    {
        WEBRTC_ERROR << "SSL_export_keying_material failed.ret: " << ret;
    }

    // key // 用于加密和解密 SRTP 数据。
    int32_t offset = 0;
    std::string client_master_key((char*)material, srtp_key_len);
    offset += srtp_key_len;
    std::string server_master_key((char*)material + offset, srtp_key_len);
    offset += srtp_key_len;
    //  salt // 用于生成会话密钥
    std::string client_master_salt((char*)material, srtp_salt_len);
    offset += srtp_salt_len;
    std::string server_master_salt((char*)material + offset, srtp_salt_len);
    offset += srtp_salt_len;

    WEBRTC_DEBUG << " dtls is server.";
    // 服务端相反
    recv_key_ = client_master_key + client_master_salt;
    send_key_ = server_master_key + server_master_salt;

    // 服务端和客户端识别错了，因为这里只做服务端，直接使用服务端的就行

    // if(is_client_)
    // {
    //     WEBRTC_DEBUG << " dtls is client.";
    //     // 客户端来说，接收者是server
    //     recv_key_ = server_master_key + server_master_salt;
    //     send_key_ = client_master_key + client_master_salt;
    // }
    // else
    // {
    //     WEBRTC_DEBUG << " dtls is server.";
    //     // 服务端相反
    //     recv_key_ = client_master_key + client_master_salt;
    //     send_key_ = server_master_key + server_master_salt;
    // }
}
