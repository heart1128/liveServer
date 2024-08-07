/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:07:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-09 11:25:24
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.cpp
 * @Description:  learn
 */
#include "WebrtcPlayUser.h"
#include "live/base/LiveLog.h"
#include "base/Config.h"
#include "live/Session.h"
#include "network/net/UdpSocket.h"
#include <random>

using namespace tmms::live;

WebrtcPlayUser::WebrtcPlayUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
:PlayerUser(ptr, stream, s), dtls_(this)
{
    local_ufrag_ = GetUFrag(8);
    local_passwd_ = GetUFrag(32);
    uint32_t audio_ssrc = GetSsrc(10);
    uint32_t video_ssrc = audio_ssrc + 1;

    // 解析的sdp是远端的，生成的是本地的
    sdp_.SetLocalUFrag(local_ufrag_);
    sdp_.SetLocalPasswd(local_passwd_);
    sdp_.SetAudioSsrc(audio_ssrc);
    sdp_.SetVideoSsrc(video_ssrc);

    // 生成证书指纹，设置
    if(!dtls_.Init())
    {
        LIVE_ERROR << "dtls init failed.";
        return;
    }
    sdp_.SetFingerprint(dtls_.Fingerprint());

    auto config = sConfigMgr->GetConfig();
    if(config)
    {
        auto serverinfo = config->GetServiceInfo("webrtc", "udp");
        if(serverinfo)
        {
            // TODO: 这里需要给sdp设置udp的发送地址，如果是0.0.0.0就发送不到，所以发送到不是同一个局域网的不知道怎么弄
            sdp_.SetServerAddr("8.149.142.249");
            // sdp_.SetServerAddr(serverinfo->addr);
            sdp_.SetServerPort(serverinfo->port);
        }
    }
    sdp_.SetStreamName(s->SessionName());
}

bool WebrtcPlayUser::PostFrames()
{
    return false;
}

UserType WebrtcPlayUser::GetUserType() const
{
    return UserType::kUserTypePlayerWebRTC;
}

/**
 * @description: 解析远端sdp
 * @param {string} &sdp
 * @return {*}
**/
bool WebrtcPlayUser::ProcessOfferSdp(const std::string &sdp)
{
    return sdp_.Decode(sdp);
}

const std::string &WebrtcPlayUser::LocalUFrag() const
{
    return sdp_.GetLocalUFrag();
}

const std::string &WebrtcPlayUser::LocalPasswd() const
{
     return sdp_.GetLocalPasswd();
}

const std::string &WebrtcPlayUser::RemoteUFrag() const
{
    return sdp_.GetRemoteUFrag();
}

std::string WebrtcPlayUser::BuildAnswerSdp()
{
    return sdp_.Encode();
}

void WebrtcPlayUser::SetConnection(const ConnectionPtr &conn)
{
    User::SetConnection(conn);
    conn->PeerAddr().GetSockAddr((struct sockaddr*)&addr_);
}

/**
 * @description: 在收到dtls数据的时候，由上层webrtcSevice调用
 * @param {char} *buf
 * @param {size_t} size
 * @return {*}
 */
void WebrtcPlayUser::OnDtlsRecv(const char *buf, size_t size)
{
    dtls_.OnRecv(buf, size);
}

void WebrtcPlayUser::OnDtlsSend(const char *data, size_t size, Dtls *dtls)
{
    LIVE_DEBUG << "dtls send size:" << size;
    packet_ = Packet::NewPacket(size);
    memcpy(packet_->Data(), data, size);
    packet_->SetPacketSize(size);
    auto socket = std::dynamic_pointer_cast<UdpSocket>(connection_);
    socket->Send(packet_->Data(), packet_->PacketSize(), (struct sockaddr*)&addr_, addr_len_);
}

/**
 * @description: dtls握手完成之后，拿到加密秘钥，就要初始化srtp
 * @param {Dtls} *dtls
 * @return {*}
 */
void WebrtcPlayUser::OnDtlsHandshakeDone(Dtls *dtls)
{
    LIVE_DEBUG << "dtls handshake done.";
    dtls_done_ = true;
    srtp_.Init(dtls_.RecvKey(), dtls_.SendKey());
}

/**
 * @description: 生成一个安全描述的用户名，或者密码
 * @param {int} size
 * @return {*}
**/
std::string tmms::live::WebrtcPlayUser::GetUFrag(int size)
{
    static std::string table = "1234567890abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string frag;

    // 初始化随机数生成器
    static std::mt19937 mt{std::random_device{}()};
    // 离散均匀分布类，指定随机数范围,不超过uint8_t, 1字节
    static std::uniform_int_distribution<> rand(0, table.size());

    for(int i = 0; i < size; ++i)
    {
        frag.push_back(table[(rand(mt) % table.size())]);
    }

    return frag;
}


/**
 * @description: 生成一个媒体源唯一标识
 * @param {int} size
 * @return {*}
**/
uint32_t WebrtcPlayUser::GetSsrc(int size)
{
    // 初始化随机数生成器
    static std::mt19937 mt{std::random_device{}()};
    static std::uniform_int_distribution<> rand(10000000,99999999);

    return rand(mt);
}
