/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 17:48:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 18:34:11
 * @FilePath: /liveServer/src/mmedia/webrtc/DtlsCerts.cpp
 * @Description:  learn 
 */
#include "DtlsCerts.h"
#include "mmedia/base/MMediaLog.h"
#include <random>

using namespace tmms::mm;

DtlsCerts::~DtlsCerts()
{
    if(dtls_pkey_)
    {
        EVP_PKEY_free(dtls_pkey_);
        dtls_pkey_ = nullptr;
    }
    if(dtls_certs_)
    {
        X509_free(dtls_certs_);
        dtls_certs_ = nullptr;
    }
}


/// @brief 这个流程创建了一个椭圆曲线密钥对，使用这个密钥对生成了一个自签名的证书，
        // 并计算了证书的SHA-256指纹，这个指纹可以用于WebRTC中的身份验证。
/// @return 
bool DtlsCerts::Init()
{
    ///////// 1. 初始化秘钥
    // 创建一个新的EVP_PKEY对象，用于存储各种类型的密钥
    dtls_pkey_ = EVP_PKEY_new();
    // 检查EVP_PKEY_new是否成功创建了密钥
    if(!dtls_pkey_)
    {
        WEBRTC_ERROR << "EVP_PKEY_new failed.";
        return false;
    }

    // 通过椭圆曲线创建一个key
    // 使用NID_X9_62_prime256v1（即NIST P-256）椭圆曲线生成新的EC_KEY对象
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_X9_62_prime256v1);
    // 检查是否成功创建了EC_KEY对象
    if(!key)
    {
        WEBRTC_ERROR << "EC_KEY_new_by_curve_name failed.";
        return false;
    }
    // 设置椭圆曲线密钥的ASN1标志，这里指定使用命名曲线
    EC_KEY_set_asn1_flag(key, OPENSSL_EC_NAMED_CURVE);

    // 生成秘钥，即生成椭圆曲线密钥对（公钥和私钥）
    auto ret = EC_KEY_generate_key(key);
    // 检查密钥生成是否成功
    if(!ret)
    {
        WEBRTC_ERROR << "EC_KEY_generate_key failed.";
        return false;
    }

    // 将生成的椭圆曲线密钥对赋值给EVP_PKEY对象
    EVP_PKEY_assign_EC_KEY(dtls_pkey_, key);

    ///////// 2. 初始化证书
    // 创建一个新的X509证书对象
    dtls_certs_ = X509_new();
    // 生成一个随机数，用作证书序列号
    auto req = GenRandom();  
    // 设置证书的序列号
    ASN1_INTEGER_set(X509_get_serialNumber(dtls_certs_), req);
    // 设置证书的有效时间，notBefore为当前时间
    X509_gmtime_adj(X509_get_notBefore(dtls_certs_), 0);
    // 设置证书的有效时间，notAfter为一年后
    X509_gmtime_adj(X509_get_notAfter(dtls_certs_), 60 * 60 * 24 * 365);

    // 创建一个新的X509名称对象，用于存储证书的主题信息
    X509_NAME * subject = X509_NAME_new();
    // 设置证书的组织名称，结合随机数生成唯一值
    const std::string &aor = "tmms.net" + std::to_string(req);
    X509_NAME_add_entry_by_txt(subject, "O", MBSTRING_ASC, (const unsigned char*)aor.c_str(), aor.size(), -1, 0); 

    // 设置证书的公用名（CN），通常为域名或IP地址
    const std::string &name = "tmms.cn";
    X509_NAME_add_entry_by_txt(subject, "CN", MBSTRING_ASC, (const unsigned char*)name.c_str(), name.size(), -1, 0); 
    // 设置证书的发行者名称
    X509_set_issuer_name(dtls_certs_, subject);
    // 设置证书的主题名称
    X509_set_subject_name(dtls_certs_, subject);

    ///////// 3. 设置到公钥（证书 + 私钥）
    // 将EVP_PKEY对象（包含公钥和私钥）设置为证书的公钥
    X509_set_pubkey(dtls_certs_, dtls_pkey_);

    ///////// 4. 签名, 用的sha1签名算法
    // 使用SHA-1哈希算法和EVP_PKEY中的私钥对证书进行签名
    X509_sign(dtls_certs_, dtls_pkey_, EVP_sha256());

    ///////// 5. 计算指纹
    // 定义一个数组用于存储证书的SHA-256摘要
    unsigned char fingerprint_bin[EVP_MAX_MD_SIZE];
    // 定义一个变量用于存储摘要的长度
    unsigned int len = 0;

    // 使用SHA-256算法计算证书的摘要
    X509_digest(dtls_certs_, EVP_sha256(), fingerprint_bin, &len);
    // 定义一个字符数组用于存储十六进制表示的指纹
    char fingerprint_result[(EVP_MAX_MD_SIZE * 3) + 1];
    // 将二进制摘要转换为十六进制字符串
    for(int i = 0; i < len; ++i)
    {
        // 将每个字节转换为两位十六进制数，并用冒号分隔
        sprintf(fingerprint_result + (i * 3), "%.2X:", fingerprint_bin[i]);
    }
    // 如果有数据，去掉最后一个冒号
    if(len > 0)
    {
        fingerprint_result[(len*3) - 1] = 0;
    }

    // 将计算出的指纹字符串赋值给fingerprint_成员变量
    fingerprint_.assign(fingerprint_result, (len*3) - 1);
    // 打印指纹信息，用于调试
    WEBRTC_DEBUG << "fingerprint: " << fingerprint_;
    // 返回true表示整个流程成功完成
    return true;


}

const std::string &DtlsCerts::Fingerprint() const
{
    return fingerprint_;
}

EVP_PKEY *DtlsCerts::GetPrivateKey() const
{
    return dtls_pkey_;
}

X509 *DtlsCerts::GetCerts() const
{
    return dtls_certs_;
}

uint32_t DtlsCerts::GenRandom()
{
    // 初始化随机数生成器
    std::mt19937 mt{std::random_device{}()};
    return mt();
}
