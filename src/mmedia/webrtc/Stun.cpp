/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-08 17:17:21
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 19:28:42
 * @FilePath: /liveServer/src/mmedia/webrtc/Stun.cpp
 * @Description:  learn 
 */
#include "Stun.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "mmedia/base/BytesWriter.h"
#include "mmedia/mpegts/TsTool.h"

using namespace tmms::mm;

/**
 * @description: 解析Stun，从header解析开始, header之后是属性
 * @param {char *} data
 * @param {uint32_t} size  stun包大小
 * @return {*}
 */
bool Stun::Decode(const char * data, uint32_t size)
{
    // 解析header
    type_ = (StunMessageType)BytesReader::ReadUint16T(data);
    data += 2;
    stun_length_ = BytesReader::ReadUint16T(data);
    data += 2;
    auto magic = BytesReader::ReadUint32T(data);
    // magic是固定的
    if(magic != kStunMagicCookie)
    {
        WEBRTC_ERROR << "stun magic:" << magic << " not equal kStunMagicCookie:" << kStunMagicCookie;
        return false;
    }
    data += 4;

    transcation_id_.assign(data, 12); // 不需要转小端
    data += 12;
    size -= 20;

    WEBRTC_DEBUG << "stun type: " << type_
                << " length: " << stun_length_
                << " transcation_id: " << transcation_id_;
    
    // 解析属性, 属性是4字节对齐的
    // 属性 ： 类型(16bit) + 长度(16bit)
    while(size >= 4)
    {
        uint16_t attr_type = BytesReader::ReadUint16T(data);
        data += 2;
        uint16_t attr_len = BytesReader::ReadUint16T(data);
        data += 2;
        size -= 4;
        // 4字节对齐的，判断下填充是不是正确
        uint16_t padding_len = (4 - attr_len % 4) % 4;
        // 剩下的包大小不够了
        if(size < padding_len + attr_len)
        {
            WEBRTC_ERROR << "stun attr len : " << attr_len
                            << " padding:" << padding_len
                            << " size:" << size;
        }

        // 解析属性值
        // 这里只用到了下面两个
        switch (attr_type)
        {
            case kStunAttrUsername:  // 用户名
            {
                user_name_.assign(data, attr_len);
                WEBRTC_DEBUG << "stun user name:" << user_name_;
                break;
            }
            case kStunAttrPassword:  // 验证消息完整性
            {
                password_.assign(data, attr_len);
                WEBRTC_DEBUG << "stun passwd:" << password_;
                break;
            }
        }

        data += attr_len + padding_len;
        size -= (attr_len + padding_len);
    }

    return true;
}


/**
 * @description: response的包，作为服务器，格式也是stun的标准格式
 * @return {*}
 */
PacketPtr Stun::Encode()
{
    PacketPtr packet = Packet::NewPacket(512);
    char *data = packet->Data();

    // 写入header
    data += BytesWriter::WriteUint16T(data, type_);
    data += BytesWriter::WriteUint16T(data, 0); // 目前还没消息，后面添加
    data += BytesWriter::WriteUint32T(data, kStunMagicCookie);
    // 这里是小端，不用大端
    std::memcpy(data, transcation_id_.c_str(), transcation_id_.size());
    data += 12;

    // 写属性
    
        // user_name
    BytesWriter::WriteUint16T(data, kStunAttrUsername); // 类型
    BytesWriter::WriteUint16T(data + 2, user_name_.size()); // 长度
    std::memcpy(data + 4, user_name_.c_str(), user_name_.size()); // 小端
    int32_t padding_bytes = (4 - user_name_.size() % 4) % 4; // 需要的padding
    if(padding_bytes != 0) 
    {
        // 4字节对齐，填充0
        const char padding = '0';
        std::memcpy(data + 4 + user_name_.size(), &padding, padding_bytes);
    }
    data += (4 + user_name_.size() + padding_bytes);

        // XormapAddress
        // 和MAPPED-ADDRESS基本相同，不同点是反射地址部分经过了一次异或（XOR）处理
    BytesWriter::WriteUint16T(data, kStunAttrXorMappedAddress);
    BytesWriter::WriteUint16T(data + 2, 8);       //属性长度
    BytesWriter::WriteUint8T(data + 4, 0);
    BytesWriter::WriteUint8T(data + 5, 0x01);
    BytesWriter::WriteUint16T(data + 6, mapped_port_ ^ (kStunMagicCookie >> 16));
    BytesWriter::WriteUint32T(data + 8, mapped_addr_ ^ kStunMagicCookie);
    data  += (4+8);

        // MessageIntegrity
        // MESSAGE-INTEGRITY属性包含STUN消息的HMAC-SHA1，它可以出现在捆绑请求或捆绑响应中；
        // MESSAGE-INTEGRITY属性必须是任何STUN消息的最后一个属性。它的内容决定了HMAC输入的Key值
    char   *data_begin = packet->Data();
    size_t  data_bytes = data - data_begin - 20;
    size_t  paylod_len = data_bytes + (4+20);
    BytesWriter::WriteUint16T(data_begin + 2, paylod_len);
    BytesWriter::WriteUint16T(data, kStunAttrMessageIntegrity);
    BytesWriter::WriteUint16T(data + 2, 20);
    CalcHmac(data + 4, data_begin, data_bytes+20);  // 这个不包含Fingerprint,老协议不支持
    // 计算完成后，恢复实际长度
    paylod_len = data_bytes + (4+20)+(4+4);
    BytesWriter::WriteUint16T(data_begin + 2, paylod_len);
    data  += (4 + 20);

        // Fingerprint , 放在最后支持新协议
        // 指纹认证，此属性可以出现在所有的 STUN 消息中，该属性用于区分 STUN 数据包与其他协议的包。
        // 属性的值为采用 CRC32 方式计算 STUN 消息直到但不包括FINGERPRINT 属性的的结果，并与 32 位的值 0x5354554e 异或。
    BytesWriter::WriteUint16T(data, kStunAttrFingerprint);
    BytesWriter::WriteUint16T(data + 2, 4);
    uint32_t crc32 = TsTool::CRC32Ieee(packet->Data(), data - packet->Data()) ^ 0x5354554e;
    BytesWriter::WriteUint32T(data + 4, crc32);
    data  += (4+4);

    packet->SetPacketSize(paylod_len+20);
    return packet;
}

/**
 * @description: user_name_是localUFrag + remoteUFrag组成的，
 * @return {*}
 */
std::string Stun::LocalUFrag()
{
    // :分割
    auto pos = user_name_.find_first_of(':');
    if(pos != std::string::npos)
    {
        return user_name_.substr(0, pos);
    }
    return std::string();
}

void Stun::SetPassword(const std::string &pwd)
{
    password_ = pwd;
}

void Stun::SetMappedAddr(uint32_t addr)
{
    mapped_addr_ = addr;
}

void Stun::SetMappedPort(uint16_t port)
{
    mapped_port_ = port;
}

void Stun::SetMessageType(StunMessageType type)
{
    type_ = type;
}

/**
 * @description: 使用sha1加密内容，计算完整性
 * @param {char} *buf
 * @param {char} *data
 * @param {size_t} bytes
 * @return {*}
 */
size_t Stun::CalcHmac(char *buf, const char *data, size_t bytes)
{
    unsigned int digestLen;
#if OPENSSL_VERSION_NUMBER > 0x10100000L
    HMAC_CTX *ctx = HMAC_CTX_new();
    HMAC_Init_ex(ctx, password_.c_str(), password_.size(), EVP_sha1(), NULL);
    HMAC_Update(ctx, (const unsigned char *)data, bytes);
    HMAC_Final(ctx, (unsigned char *)buf, &digestLen);
    HMAC_CTX_free(ctx);
#else
    HMAC_CTX ctx;
    HMAC_CTX_init(&ctx);
    HMAC_Init_ex(&ctx, password_.c_str(), password_.size(), EVP_sha1(), NULL);
    HMAC_Update(&ctx, (const unsigned char *)data, bytes);
    HMAC_Final(&ctx, (unsigned char *)buf, &digestLen);
    HMAC_CTX_cleanup(&ctx);
#endif
    return digestLen;
}
