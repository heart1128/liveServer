/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 18:56:39
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-11 15:17:52
 * @FilePath: /liveServer/src/mmedia/webrtc/Sdp.cpp
 * @Description:  learn 
 */
#include "Sdp.h"
#include "mmedia/base/MMediaLog.h"
#include "base/StringUtils.h"

using namespace tmms::mm;

/**
 * @description: SDP的格式是一行一个字段，按分行符进行划分
 *          这里只解析了  音视频类型
 *                      ice
 *                      fingerprint
 */ 
bool Sdp::Decode(const std::string &sdp)
{
    auto list = base::StringUtils::SplitString(sdp, "\n");
    if(list.size() == 0)
    {
        return false;
    }

    // 定义音视频的tag
    static const std::string &rtpmap_token = "a=rtpmap:";
    // ice 用户名和密码 tag
    static const std::string &ice_ufrag_token = "a=ice-ufrag:";
    static const std::string &ice_pwd_token = "a=ice-pwd:";

    static const std::string &fingerprint_token = "a=fingerprint:";

    for(auto line : list)
    {
        // 根据前缀取值
        if(base::StringUtils::StartsWith(line, ice_ufrag_token))
        {
            remote_ufrag_ = line.substr(ice_ufrag_token.size());
            WEBRTC_DEBUG << "remote ufrag: " << remote_ufrag_; 
        }
        else if(base::StringUtils::StartsWith(line, ice_pwd_token))
        {
            remote_passwd_ = line.substr(ice_pwd_token.size());
            WEBRTC_DEBUG << "remote passwd: " << remote_passwd_; 
        }
        else if(base::StringUtils::StartsWith(line, fingerprint_token))
        {
            remote_fingerprint_ = line.substr(fingerprint_token.size());
            WEBRTC_DEBUG << "remote fingerprint: " << remote_fingerprint_; 
        }
        else if(base::StringUtils::StartsWith(line, rtpmap_token))
        {
            // 音视频
            // a=rtpmap:111 opus/48000/2\r\na=rtcp-fb:111 transport-cc
            std::string content = line.substr(rtpmap_token.size());
            // 第一个是动态类型，没找到就是非法的
            // 动态类型的值是0-127之间的数字，动态通常是96-127
            auto pos = content.find_first_of(" ");
            if(pos == std::string::npos)
            {
                continue;
            }
            int32_t pt = std::atoi(content.substr(0, pos).c_str());

            // 编码器名字
            auto pos1 = content.find_first_of("/", pos + 1);
            if(pos1 == std::string::npos)
            {
                continue;
            }
            std::string name = content.substr(pos + 1, pos1 - pos - 1);
                // 根据编码器名称判断是音频还是视频
                // 音频使用OPUS
            if(audio_payload_type_ == - 1 && name == "opus")
            {
                audio_payload_type_ = pt;
                WEBRTC_DEBUG << "audio_payload_type: " << audio_payload_type_;
            }
            else if(video_payload_type_ == -1 && name == "H264")
            {
                video_payload_type_ = pt;
                WEBRTC_DEBUG << "video_payload_type: " << video_payload_type_;
            }
        }   
        
    }

    return true;
}

const std::string &Sdp::GetRemoteUFrag() const
{
    return remote_ufrag_;
}

const std::string &Sdp::GetFingerprint() const
{
    return local_fingerprint_;
}

int32_t Sdp::GetVideoPayloadType() const
{
    return video_payload_type_;
}

int32_t Sdp::GetAudioPayloadType() const
{
    return audio_payload_type_;
}

void Sdp::SetFingerprint(const std::string &fp)
{
    local_fingerprint_ = fp;
}

/// @param name   session_name
void Sdp::SetStreamName(const std::string &name)
{
    auto list = base::StringUtils::SplitString(name, "/");
    if(list.size() == 3)
    {
        stream_name_ = list[2];
    }
    else
    {
        stream_name_ = name;
    }
}

void Sdp::SetLocalUFrag(const std::string &frag)
{
    local_ufrag_ = frag;
}

void Sdp::SetLocalPasswd(const std::string &pwd)
{
    local_passwd_ = pwd;
}

void Sdp::SetServerPort(uint16_t port)
{
    server_port_ = port;
}

void Sdp::SetServerAddr(const std::string &addr)
{
    server_addr_ = addr;
}

void Sdp::SetVideoSsrc(uint32_t ssrc)
{
    video_ssrc_ = ssrc;
}

void Sdp::SetAudioSsrc(int32_t ssrc)
{
    audio_ssrc_ = ssrc;
}

const std::string &Sdp::GetLocalPasswd() const
{
    return local_passwd_;
}

const std::string &Sdp::GetLocalUFrag() const
{
    return local_ufrag_;
}

uint32_t Sdp::VideoSsrc() const
{
    return video_ssrc_;
}

uint32_t Sdp::AudioSsrc() const
{
    return audio_ssrc_;
}

std::string Sdp::Encode()
{
    // 1. 生成会话描述
    std::ostringstream ss;

    ss << "v=0\n"; // 版本号
    ss << "o=tmms 701762458683606775 2 IN IP4 0.0.0.0\n"; // 会话发起者
    ss << "s=" << stream_name_ << "\n";  // 会话名
    ss << "c=IN IP4 0.0.0.0\n";   // 会话连接ip
    ss << "t=0 0\n"; // 时长
    ss << "a=group:BUNDLE 0 1\n"; // 复用连接给音视频
    ss << "a=msid-semantic: WMS " << stream_name_ << "\n";

    // 音视频描述
        // 有音频之前必须要有视频
    if(video_payload_type_ != -1 && audio_payload_type_ != -1)
    {
        // 音频
        ss << "m=audio 9 UDP/TLS/RTP/SAVPF " << audio_payload_type_ << "\n";
        ss << "a=rtpmap:"<<audio_payload_type_<<" opus/48000/2\n";
        ss << "a=fmtp:"<<audio_payload_type_<<" minptime=10;stereo=1;useinbandfec=1\n";
        ss << "c=IN IP4 0.0.0.0\n";
        ss << "a=ice-ufrag:" << local_ufrag_ << "\n";
        ss << "a=ice-pwd:" << local_passwd_ << "\n";
        ss << "a=fingerprint:sha-256 " << local_fingerprint_ << "\n";
        ss << "a=setup:passive\n";
        ss << "a=mid:0\n";
        ss << "a=sendonly\n";
        ss << "a=rtcp-mux\n";
        ss << "a=rtcp-rsize\n";
        ss << "a=rtcp-fb:"<<audio_payload_type_<<" transport-cc\n";
        ss << "a=rtcp-fb:"<<audio_payload_type_<<" nack\n";
        ss << "a=ssrc:"<<audio_ssrc_ << " cname:" << stream_name_ << "\n";
        ss << "a=ssrc:"<<audio_ssrc_ << " msid:" << stream_name_<< " "<<stream_name_<<"_audio\n";
        ss << "a=ssrc:"<<audio_ssrc_ << " mslabel:" << stream_name_<< "\n";
        ss << "a=ssrc:"<<audio_ssrc_ << " label:"<<stream_name_<<"_audio\n";
        ss << "a=candidate:0 1 udp 2130706431 "<<server_addr_<< " "<<server_port_ <<" typ host generation 0\n";

    }
    if(video_payload_type_ != -1)
    {
        // 视频
        ss << "m=video 9 UDP/TLS/RTP/SAVPF "<< video_payload_type_ << "\n";
        ss << "a=rtpmap:"<<video_payload_type_<<" H264/90000\n";
        ss << "c=IN IP4 0.0.0.0\n";
        ss << "a=ice-ufrag:" << local_ufrag_ << "\n";
        ss << "a=ice-pwd:" << local_passwd_ << "\n";
        ss << "a=fingerprint:sha-256 " << local_fingerprint_ << "\n";
        ss << "a=setup:passive\n";
        ss << "a=mid:1\n";
        ss << "a=sendonly\n";
        ss << "a=rtcp-mux\n";
        ss << "a=rtcp-rsize\n";
        ss << "a=rtcp-fb:"<<video_payload_type_<<" ccm fir\n";
        ss << "a=rtcp-fb:"<<video_payload_type_<<" goog-remb\n";
        ss << "a=rtcp-fb:"<<video_payload_type_<<" nack\n";
        ss << "a=rtcp-fb:"<<video_payload_type_<<" nack pli\n";
        ss << "a=rtcp-fb:"<<video_payload_type_<<" transport-cc\n";
        ss << "a=ssrc:"<<video_ssrc_ <<" cname:" << stream_name_ << "\n";
        ss << "a=ssrc:"<<video_ssrc_ <<" msid:"<<stream_name_<<" "<<stream_name_<<"_video\n";
        ss << "a=ssrc:"<<video_ssrc_ <<" mslabel:"<< stream_name_ << "\n";
        ss << "a=ssrc:"<<video_ssrc_ <<" label:"<<stream_name_<<"_video\n";
        ss << "a=candidate:0 1 udp 2130706431 "<<server_addr_<< " "<<server_port_ <<" typ host generation 0\n";
    }

    return ss.str();
}
