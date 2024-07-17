#include "HLSMuxer.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/AVTypes.h"
#include "base/StringUtils.h"

using namespace tmms::mm;

HLSMuxer::HLSMuxer(const std::string &session_name)
{
    auto list = base::StringUtils::SplitString(session_name, "/");
    if(list.size() == 3)
    {
        stream_name_ = list[2];
    }
}

/// @brief 获取m3u8内容
/// @return 
std::string HLSMuxer::PlayList()
{
    return fragment_window_.GetPlayList();
}


bool HLSMuxer::IsCodecHeader(const PacketPtr &packet)
{
    // 第一个字节是FLV
    if(packet->PacketSize() > 1)
    {
        const char *b = packet->Data() + 1;
        // 音频流的AAC第二个字节为0 
        // 视频流的第二个字节为0，就是H.264的
        if(*b == 0)
        {
            return true;
        }
    }
    return false;
}

/// @brief 组装hls切片包
/// @param packet  
void HLSMuxer::OnPacket(PacketPtr &packet)
{
    if(current_fragment_)
    {
        // 1. 遇到关键帧，并且当前时间超过了最小时间 2. 与关键帧无关，只要超过最大定义时间就切片
        auto is_key = packet->IsKeyFrame();
        if((is_key && current_fragment_->Duration() >= min_fragment_size_)
            || current_fragment_->Duration() >= max_fragment_size_)
        {
            fragment_window_.AppendFragment(std::move(current_fragment_));
            current_fragment_.reset();
        }
    }

    if(!current_fragment_)
    {
        current_fragment_ = fragment_window_.GetIdleFragment(); // 没有就申请一个
        // 还是申请失败，就自己主动申请一个
        if(!current_fragment_)
        {
            current_fragment_ = std::make_shared<Fragment>();
        }
        current_fragment_->Reset();
        current_fragment_->SetBaseFileName(stream_name_);
        current_fragment_->SetSequenceNo(fragmemt_seq_no_++); // 有序
        // 每个头开始都要先写pat pmt
        encoder_.WritePatPmt(current_fragment_.get());
    }

    // 是编码头就单独解析
    // 会设置pmt pat
    if(IsCodecHeader(packet))
    {
        ParseCodec(current_fragment_, packet);
    }

    // 开始从flv解析的数据组装成mepgets
    int64_t dts = packet->TimeStamp();
    encoder_.Encode(current_fragment_.get(), packet, dts);
}

FragmentPtr HLSMuxer::GetFragment(const std::string &name)
{
    // 根据文件名查询
    return fragment_window_.GetFragmentByName(name);
}

void HLSMuxer::ParseCodec(FragmentPtr &fragment, PacketPtr &packet)
{
    char *data = packet->Data(); // Audio tags

    if(packet->IsAudio())
    {
        AudioCodecID id = (AudioCodecID)((*data & 0xf0) >> 4); // 高4位表示了音频类型MP3 aac
        encoder_.SetStreamType(fragment.get(), kVideoCodecIDReserved, id);
    }
    if(packet->IsVideo())
    {
        VideoCodecID id = (VideoCodecID)((*data & 0x0f) >> 4); // 低四位表示了音频类型MP3 aac
        encoder_.SetStreamType(fragment.get(),id, kAudioCodecIDReserved);
    }
}
