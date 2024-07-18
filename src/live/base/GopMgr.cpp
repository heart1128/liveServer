/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 16:51:45
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-28 17:07:30
 * @FilePath: /liveServer/src/live/GopMgr.cpp
 * @Description:  learn 
 */
#include "GopMgr.h"
#include "live/base/LiveLog.h"
#include <sstream>

using namespace tmms::live;

GopMgr::GopMgr()
{
}

GopMgr::~GopMgr()
{
}

void GopMgr::AddFrame(const PacketPtr &packet)
{
    lastest_timestamp_ = packet->TimeStamp();

    // 通过包携带的类型判断是不是关键帧
    if(packet->IsKeyFrame())
    {
        // 保存关键帧
        gops_.emplace_back(packet->Index(), packet->TimeStamp());
        max_gop_length_ = std::max(max_gop_length_, gop_length_);
        total_gop_length_ += gop_length_;
        // 到了关键帧，表示上一帧结束了，计算新的帧长
        gop_length_ = 0;
        gop_numbers_++; // 完成一个帧，加+
    }
    gop_length_++;
}

int32_t GopMgr::MaxGopLength() const
{
    return max_gop_length_;
}

size_t GopMgr::GopSize() const
{
    return gops_.size();
}

/// @brief 找到最接近content_latency延时间隔的i帧（下一帧）
/// @param content_latency   配置文件定义好的合适的两帧延时
/// @param latency 
/// @return  返回找到的gop的index
int GopMgr::GetGopByLatency(int content_latency, int &latency) const
{
    int got = -1;
    latency = 0;
    auto iter = gops_.rbegin();
    // 找到最接近content_latency延时间隔的i帧
    // 因为这个延时下两帧之间保存的gop包最合适，可以抗抖动
    for(; iter != gops_.rend(); ++iter)
    {
        // 计算延时，最新的帧延时 - 查找的延时
        int item_latency = lastest_timestamp_ - iter->timestamp;
        if(item_latency < content_latency)
        {
            got = iter->index;
            latency = item_latency;
        }
        else
        {
            break;
        }
    }
    return got;
}

/// @brief 根据给定的idx清理之前的indx
/// @param min_idx 
void GopMgr::ClearExpriedGop(int min_idx)
{
    if(gops_.empty())
    {
        return;
    }

    for(auto iter = gops_.begin(); iter != gops_.end();)
    {
        if(iter->index <= min_idx)
        {
            iter = gops_.erase(iter); // 小于设定的idx就是过期的，删除返回下一个迭代器
        }
        else
        {
            ++iter; // 否则就是没有过期的，下一个
        }
    }
}

void GopMgr::PrintAllGop()
{
    std::stringstream ss;

    ss << "all gop:";

    for(auto iter = gops_.begin(); iter != gops_.end(); ++iter)
    {
        ss << "[" << iter->index << "," << iter->timestamp << "]";
    }
    LIVE_TRACE << ss.str() << "\n";
}
