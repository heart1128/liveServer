/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 16:07:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-19 17:00:29
 * @FilePath: /liveServer/src/live/user/WebrtcPlayUser.cpp
 * @Description:  learn
 */
#include "WebrtcPlayUser.h"
#include "live/base/LiveLog.h"
#include "base/Config.h"
#include "live/Session.h"
#include "network/net/UdpSocket.h"
#include "live/LiveService.h"
#include "live/Stream.h"
#include "base/TTime.h"
#include "mmedia/rtp/SenderReport.h"
#include "mmedia/rtp/Rtpfb.h"
#include <sys/time.h>
#include <random>

using namespace tmms::live;

WebrtcPlayUser::WebrtcPlayUser(const ConnectionPtr &ptr, const StreamPtr &stream, const SessionPtr &s)
:PlayerUser(ptr, stream, s), dtls_(this), video_queue_(500),audio_queue_(500)
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
            sdp_.SetServerAddr("10.101.128.69");
            // sdp_.SetServerAddr(serverinfo->addr);
            sdp_.SetServerPort(serverinfo->port);
        }
    }
    sdp_.SetStreamName(s->SessionName());
}

/**
 * @description: 发送数据帧，通过上层的OnActive调用user进行发送
 * @return {*}
 */
bool WebrtcPlayUser::PostFrames()
{
    // 流没有准备好或者元数据都没有发送, dtls没有握手成功也不行
    if(!stream_->Ready() || ! stream_->HasMedia() || !dtls_done_)
    {
        Deactive();
        return false;
    }

    stream_->GetFrames(std::dynamic_pointer_cast<PlayerUser>(shared_from_this()));
    meta_.reset();
    std::list<PacketPtr> rtp_pkts;
    // 头
    if(audio_header_)
    {
        auto ret = rtp_muxer_.EncodeAudio(audio_header_, rtp_pkts, 0);
        audio_header_.reset();
    }
    else if(video_header_)
    {
        auto ret = rtp_muxer_.EncodeVideo(video_header_, rtp_pkts, 0);
        video_header_.reset();
    }
    
    if(!out_frames_.empty()) // 发送音视频数据
    {
        for(auto &p : out_frames_)
        {
            // 找到了关键帧
            if(p->IsVideo() && p->IsKeyFrame())
            {
                got_key_frame_ = true;
            }
            if(!got_key_frame_)
            {
                continue;
            }
            // 找到了关键帧， 发送数据
            if(p->IsAudio())
            {
                rtp_muxer_.EncodeAudio(p, rtp_pkts, p->TimeStamp());
            }
            else if(p->IsVideo())
            {
                rtp_muxer_.EncodeVideo(p, rtp_pkts, p->TimeStamp());
            }
        }
        out_frames_.clear();
    }
    // 全部rtp包都放在了这里，进行发送
    if(!rtp_pkts.empty())
    {
        std::list<PacketPtr> result;
        for(auto &p : rtp_pkts)
        {
            // 统计发送的包数量，放入sr报文段
            bool is_video = true;
            if(p->IsAudio())
            {
                is_video = false;
                // 保存到队列
                AddAudio(p);
            }
            else
            {
                AddVideo(p);
            }

            // 发送之前使用srtp进行加密（也就是dtls握手生成的秘钥加密）
            auto np = srtp_.RtpProtect(p);
            if(np)
            {
                // 统计音视频字节数和包数量
                if(is_video)
                {
                    video_out_bytes_ += np->PacketSize();
                    ++video_out_pkts_count_;
                }
                else
                {
                    audio_out_bytes_ += np->PacketSize();
                    ++audio_out_pkts_count_;
                }

                np->SetExt(addr_);
                result.emplace_back(np);
            }
        }
        // 获取webserver进行发送
        auto server = sLiveService->GetWebrtcServer();
        server->SendPacket(result);
        // 检测要不要发送sr, 一个周期内
        CheckSR();
    }
    else
    {
        // 没有数据了
        Deactive();
    }
    return true;
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



/**
 * @description: 在dtls中调用，发送dtls握手的数据
 * @param {char} *data
 * @param {size_t} size
 * @param {Dtls} *dtls
 * @return {*}
 */
void WebrtcPlayUser::OnDtlsSend(const char *data, size_t size, Dtls *dtls)
{
    LIVE_DEBUG << "dtls send size:" << size;
    PacketPtr packet = Packet::NewPacket(size);
    memcpy(packet->Data(), data, size);
    packet->SetPacketSize(size);
    // 要设置addr，之后要拿出来使用
    packet->SetExt(addr_);
    auto server = sLiveService->GetWebrtcServer();
    server->SendPacket(packet);

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

    // sdp商量好了这些消息
    // 初始化
    rtp_muxer_.Init(sdp_.GetVideoPayloadType(),
                    sdp_.GetAudioPayloadType(),
                    sdp_.VideoSsrc(),
                    sdp_.AudioSsrc());
}

/**
 * @description: 生成一个安全描述的用户名，或者密码
 * @param {int} size
 * @return {*}
**/
std::string WebrtcPlayUser::GetUFrag(int size)
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


void WebrtcPlayUser::SendSR(bool is_video)
{
    uint64_t now = base::TTime::NowMS();
    uint64_t elapse = 0;
    if(is_video)
    {
        elapse = now - video_sr_timestamp_;
    }
    else
    {
        elapse = now - audio_sr_timestamp_;
    }

    if(elapse < 3000) // 3s为一个周期
    {
        return;
    }

    SenderReport sr;
    
    struct timeval tv;
    gettimeofday(&tv, NULL);                        // 获取精确时间到结构体
    uint64_t ntp = tv.tv_sec * 1000000 + tv.tv_usec;    // 一个是s,一个是us，换算成s
    // 发送端的时间戳
    sr.SetNtpTimestamp(ntp);

    if(is_video)
    {
        sr.SetSsrc(rtp_muxer_.VideoSsrc());
        // 发送端最近发送rtp的时间戳
        sr.SetRtpTimestamp(rtp_muxer_.VideoTimestamp());
        sr.SetSentBytes(video_out_bytes_);
        sr.SetSentPacketCount(video_out_pkts_count_);
    }
    else
    {
        sr.SetSsrc(rtp_muxer_.AudioSsrc());
        // 发送端最近发送rtp的时间戳
        sr.SetRtpTimestamp(rtp_muxer_.AudioTimestamp());
        sr.SetSentBytes(audio_out_bytes_);
        sr.SetSentPacketCount(audio_out_pkts_count_);
    }

    auto packet = sr.Encode();
    if(packet)
    {
        // srtp加密发送
        auto np = srtp_.RtcpProtect(packet);
        if(np)
        {
            np->SetExt(addr_);
            auto server = sLiveService->GetWebrtcServer();
            server->SendPacket(packet);
        }
    }

    // 更新发送时间
    if(is_video)
    {
        video_sr_timestamp_ = now;
    }
    else
    {
        audio_sr_timestamp_ = now;
    }

}


void WebrtcPlayUser::CheckSR()
{
    SendSR(true);  // 发送视频
    SendSR(false); // 发送音频
}


/**
 * @description: 把视频包加入到队列，使用下标编号，之后可以通过下标进行重传
 * @param {PacketPtr} &pkt
 * @return {*}
 */
void WebrtcPlayUser::AddVideo(const PacketPtr &pkt)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = pkt->Index() % video_queue_.size();
    video_queue_[index] = pkt;
}

void WebrtcPlayUser::AddAudio(const PacketPtr &pkt)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = pkt->Index() % audio_queue_.size();
    audio_queue_[index] = pkt;
}

PacketPtr WebrtcPlayUser::GetVideo(int idx)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = idx % video_queue_.size();
    auto pkt = video_queue_[index];
    if(pkt && pkt->Index() == idx)
    {
        return pkt;
    }
    return PacketPtr();
}

PacketPtr WebrtcPlayUser::GetAudio(int idx)
{
    std::lock_guard<std::mutex> lk(queue_lock_);
    int index = idx % audio_queue_.size();
    auto pkt = audio_queue_[index];
    if(pkt && pkt->Index() == idx)
    {
        return pkt;
    }
    return PacketPtr();
}


void WebrtcPlayUser::OnRtcp(const char *buf, size_t size)
{
    // 收到的包是srtp加密的，进行解密
    auto np = srtp_.SrtcpUnprotect(buf, size);
    if(!np)
    {
        return;
    }
    uint8_t pt = (uint8_t)buf[1];
    // rtcp有多种不同的信息反馈
    switch (pt)
    {
        case kRtcpPtRR:
        {
            break;
        }
        case kRtcpPtRtpfb:
        {
            ProcessRtpfb(np->Data(), np->PacketSize());
            break;
        }
        case kRtcpPtPsfb:
        {
            break;
        }
        default:
            break;
    }
}

/**
 * @description: 处理RTCP的fb报文
 * @param {char} *buf
 * @param {size_t} size
 * @return {*}
 */
void WebrtcPlayUser::ProcessRtpfb(const char *buf, size_t size)
{
    Rtpfb rtpfb;
    rtpfb.Decode(buf, size);

    uint32_t media_ssrc = rtpfb.MediaSsrc();
    bool is_video = media_ssrc == rtp_muxer_.VideoSsrc();
    bool is_audio = media_ssrc == rtp_muxer_.AudioSsrc();

    if(!is_video && !is_audio)
    {
        return;
    }

    // 查找丢失的报文
    auto lost_sets = rtpfb.LostSeqs();
    std::list<PacketPtr> lost_list;
    for(auto &l : lost_sets)
    {
        if(is_audio)
        {
            auto pkt = GetAudio(l);
            if(pkt)
            {
                lost_list.emplace_back(pkt);
            }
        }
        else if(is_video)
        {
            auto pkt = GetVideo(l);
            if(pkt)
            {
                lost_list.emplace_back(pkt);
            }
        }
    }

    // 存在丢失的报文
    if(!lost_list.empty())
    {
        std::list<PacketPtr> result;
        for(auto &p : lost_list)
        {
            // 加密
            auto np = srtp_.RtcpProtect(p);
            if(np)
            {
                np->SetExt(addr_);
                result.emplace_back(np);
            }
        }
        // 加密之后发送
        if(!result.empty())
        {
            auto server = sLiveService->GetWebrtcServer();
            server->SendPacket(result);
        }
    }
}
