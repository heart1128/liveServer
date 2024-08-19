/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-09 16:08:50
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 16:13:10
 * @FilePath: /liveServer/src/mmedia/webrtc/Srtp.cpp
 * @Description:  learn 
 */
#include "Srtp.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;

namespace
{
    static PacketPtr null_packet;
}

/**
 * @description: 初始化srtp库
 * @return {*}
 */
bool Srtp::InitSrtpLibrary()
{
    // 如果不调用版本号函数，就会创建失败
    WEBRTC_INFO << "srtp libray version: "<< srtp_get_version();
    // 初始化
    auto ret = srtp_init();
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp init failed.";
        return false;
    }
    // 注册时间发生回调，里面可以检测行为
    ret = srtp_install_event_handler(Srtp::OnSrtpEvent);
    return true;
}

/**
 * @description: 初始化srtp过程参数
 * @param {string} &recv_key    在dtls中拿的
 * @param {string} &send_key
 * @return {*}
 */
bool Srtp::Init(const std::string &recv_key, const std::string &send_key)
{
    srtp_policy_t srtp_policy;
    memset(&srtp_policy, 0x00, sizeof(srtp_policy_t));

    // 设置算法，加密是AES，认证用HMAC(SHA-1)
    // 加密rtp和rtcp
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&srtp_policy.rtp);
    srtp_crypto_policy_set_aes_cm_128_hmac_sha1_80(&srtp_policy.rtcp);

    srtp_policy.ssrc.value = 0;
    srtp_policy.window_size = 4096;
    srtp_policy.next = NULL;
    srtp_policy.allow_repeat_tx = 1;

    srtp_policy.ssrc.type = ssrc_any_inbound; // 输入
    srtp_policy.key = (unsigned char*)recv_key.c_str();
    auto ret = srtp_create(&recv_ctx_, &srtp_policy);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_create recv ctx failed.err= "<< ret;
        return false;
    }

    srtp_policy.ssrc.type = ssrc_any_outbound; // 输出
    srtp_policy.key = (unsigned char*)send_key.c_str();
    ret = srtp_create(&send_ctx_, &srtp_policy);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_create recv ctx failed.err= "<< ret;
        return false;
    }
    return true;
}

/**
 * @description: 对传入的数据进行rtsp加密
 * @param {PacketPtr} &pkt
 * @return {*} 返回加密后的数据
 */
PacketPtr Srtp::RtpProtect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    // SRTP_MAX_TAG_LEN 尾部
    if(bytes + SRTP_MAX_TAG_LEN >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large, bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }  

    // 传入改变，所以要用一个中间缓存
    std::memcpy(w_buffer_, pkt->Data(), bytes);
    // 对数据进行加密
    auto ret = srtp_protect(send_ctx_, w_buffer_, &bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_protect failed.err= "<< ret;
        return null_packet;
    } 

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(), w_buffer_, bytes);
    npkt->SetPacketSize(bytes);
    
    return npkt;
}

/**
 * @description: 对rtcp数据进行加密
 * @param {PacketPtr} &pkt
 * @return {*}
 */
PacketPtr Srtp::RtcpProtect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    // SRTP_MAX_TAG_LEN 尾部，发送的要加
    if(bytes + SRTP_MAX_TAG_LEN >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large, bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }  

    // 传入改变，所以要用一个中间缓存
    std::memcpy(w_buffer_, pkt->Data(), bytes);
    // 对数据进行加密
    auto ret = srtp_protect_rtcp(send_ctx_, w_buffer_, &bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_protect_rtcp failed.err= "<< ret;
        return null_packet;
    } 

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(), w_buffer_, bytes);
    npkt->SetPacketSize(bytes);
    
    return npkt;
}

/**
 * @description: rtp数据的解密
 * @param {PacketPtr} &pkt
 * @return {*}
 */
PacketPtr Srtp::SrtpUnprotect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large, bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }  

    // 传入改变，所以要用一个中间缓存
    std::memcpy(r_buffer_, pkt->Data(), bytes);
    // 对数据进行解密
    auto ret = srtp_unprotect(recv_ctx_, r_buffer_, &bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect failed.err= "<< ret;
        return null_packet;
    } 

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(), r_buffer_, bytes);
    npkt->SetPacketSize(bytes);
    
    return npkt;
}

PacketPtr Srtp::SrtcpUnprotect(PacketPtr &pkt)
{
    int32_t bytes = pkt->PacketSize();

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large, bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }  

    // 传入改变，所以要用一个中间缓存
    std::memcpy(r_buffer_, pkt->Data(), bytes);
    // 对数据进行解密
    auto ret = srtp_unprotect_rtcp(recv_ctx_, r_buffer_, &bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect failed.err= "<< ret;
        return null_packet;
    } 

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(), r_buffer_, bytes);
    npkt->SetPacketSize(bytes);
    
    return npkt;
}

PacketPtr Srtp::SrtcpUnprotect(const char *buf, size_t size)
{
    int32_t bytes = size;

    if(bytes >= kSrtpMaxBufferSize)
    {
        WEBRTC_ERROR << "pkt too large, bytes:" << bytes << " max:" << kSrtpMaxBufferSize;
        return null_packet;
    }  

    // 传入改变，所以要用一个中间缓存
    std::memcpy(r_buffer_, buf, size);
    // 对数据进行解密
    auto ret = srtp_unprotect_rtcp(recv_ctx_, r_buffer_, &bytes);
    if(ret != srtp_err_status_ok)
    {
        WEBRTC_ERROR << "srtp_unprotect failed.err= "<< ret;
        return null_packet;
    } 

    PacketPtr npkt = Packet::NewPacket(bytes);
    std::memcpy(npkt->Data(), r_buffer_, bytes);
    npkt->SetPacketSize(bytes);
    
    return npkt;
}

void Srtp::OnSrtpEvent(srtp_event_data_t *data)
{
}
