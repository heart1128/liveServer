#include "Fragment.h"
#include "mmedia/base/MMediaLog.h"
#include "base/TTime.h"

using namespace tmms::mm;

void Fragment::AppendTimeStamp(int64_t dts)
{
    if(start_dts_ == -1)
    {
        start_dts_ = dts;
    }

    // 时间间隔就是当前时间 - 开始的时间
    start_dts_ = std::min(start_dts_, dts);
    duration_ = dts - start_dts_;
}

int32_t Fragment::Write(void *buf, uint32_t size)
{
    // 先申请空间
    if(!data_)
    {
        data_ = Packet::NewPacket(buf_size_);
    }

    // 空间不足进行扩容，直到满足空间
    if(data_->Space() < size)
    {
        buf_size_ += kFragmentStepSize;
        while(data_size_ + size > buf_size_)    
        {
            buf_size_ += kFragmentStepSize;
        }

        // 旧数据复制
        PacketPtr new_pkt = Packet::NewPacket(buf_size_);
        memcpy(new_pkt->Data(), data_->Data(), data_->PacketSize());
        data_ = new_pkt;
    }

    memcpy(data_->Data() + data_->PacketSize(), buf, size);

    return size;
}

char *Fragment::Data()
{   
    // 返回可以写入的位置
    return data_->Data() + data_->PacketSize();
}

int Fragment::Size()
{
    return data_->PacketSize();
}

int64_t Fragment::Duration() const
{
    return duration_;
}

const std::string &Fragment::FileName() const
{
    return filename_;
}


/// @brief 文件名_time.ts
/// @param v 
void Fragment::SetBaseFileName(const std::string &v)
{
    filename_.clear();
    filename_.append(v);
    filename_.append("_");
    filename_.append(std::to_string(base::TTime::NowMS()));
    filename_.append(".ts");
}

int32_t Fragment::SequenceNo() const
{
    return sequence_no_;
}

void Fragment::SetSequenceNo(int32_t no)
{
    sequence_no_ = no;
}

void Fragment::Reset()
{
    duration_ = 0;
    sequence_no_ = 0;
    data_size_ = 0;
    start_dts_ = -1;
    if(data_)
    {
        data_->SetPacketSize(0);
    }
}

PacketPtr &Fragment::FragmentData()
{
    return data_;
}

void Fragment::Save()
{
}
