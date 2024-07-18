#include "PullerRelay.h"
#include "live/base/LiveLog.h"
#include "live/LiveService.h"
#include "base/Target.h"
#include "base/StringUtils.h"
#include "live/relay/pull/RtmpPuller.h"
#include "live/LiveService.h"

using namespace tmms::live;


PullerRelay::PullerRelay(Session &s)
:session_(s)
{
}

PullerRelay::~PullerRelay()
{
    ClearPuller();
}

void PullerRelay::StartPullStream()
{
    auto ret = GetTargets();
    // 没有回源配置，通过直播业务层关闭
    if(!ret)
    {
        PULLER_ERROR << " no target found.";
        sLiveService->CloseSession(session_.SessionName());
    }
    // 调用rtmpPuller回源
    Pull();
}

void PullerRelay::OnPullSucess()
{
}

void PullerRelay::OnPullClose()
{
    Pull(); // 失败尝试别的源
}

/// @brief 从appinfo中读取列表
/// @return 
bool PullerRelay::GetTargets()
{
    auto appinfo = session_.GetAppInfo();
    if(appinfo)
    {
        targets_ = appinfo->pulls;
        if(targets_.size() > 0)
        {
            return true;
        }
    }

    return false;
}

/// @brief 根据协议创建puller
/// @param p 
/// @return 
Puller *PullerRelay::GetPuller(TargetPtr p)
{
    current_loop_ = sLiveService->GetNextLoop();
    if(p->protocol == "RTMP" || p->protocol == "rtmp")
    {
        // rtmp各种服务都会通知上层，也就是这里
        return new RtmpPuller(current_loop_, &session_, this);
    }
    return nullptr;
}

void PullerRelay::SelectTarget()
{
    // 当前的target重试没有结束
    if(current_target_ && current_target_->retry < current_target_->max_retry)
    {
        current_target_->retry++;
        PULLER_DEBUG << "current targer:" << current_target_->session_name
                    << " index:" << cur_target_index_
                    << " retry: " << current_target_->retry
                    << " max retry:" << current_target_->max_retry;
        return;
    }

    // 重试超次数了，选择一个新的target
    cur_target_index_++;
    if(cur_target_index_ < targets_.size())
    {
        current_target_ = targets_[cur_target_index_];
        ClearPuller();
        // 没有设置流名，用session name设置
        if(current_target_->stream_name.empty())
        {
            auto list = base::StringUtils::SplitString(session_.SessionName(), "/");
            if(list.size() == 3)
            {
                current_target_->stream_name = list[2];
            }
        }
        puller_ = GetPuller(current_target_);
        PULLER_DEBUG << "select index:" << cur_target_index_;
    }
    else
    {
        PULLER_ERROR << "try all targets, but no stream found.";
        ClearPuller();
        sLiveService->CloseSession(session_.SessionName());
    }
}

/// @brief 开始回源
void PullerRelay::Pull()
{
    // 在推流就不需要回源了，已经有推流了
    if(session_.IsPublishing())
    {
        return ;
    }
    SelectTarget();

    // 第一次尝试，或者设定为回源没有间隔，不断禅尝试
    if(current_target_->retry == 0 || current_target_->interval == 0)
    {
        // 这个Puller就是RtmpPuller
        if(puller_)
        {
            puller_->Pull(current_target_);
        }
    }
    else
    {
        Puller *p = puller_;
        TargetPtr t = current_target_;
        // 定时到下次回源时间
        current_loop_->RunAfter(current_target_->interval,[p, t](){
            if(p)
            {
                p->Pull(t);
            }
        });
    }
}

void PullerRelay::ClearPuller()
{
    if(!puller_)
    {
        return;
    }
    // 不能在擅长其他loop的puller
    if(!current_loop_ || current_loop_->IsInLoopThread())
    {
        delete puller_;
        puller_ = nullptr;
    }
    else
    {
        Puller *p = puller_;
        current_loop_->RunInLoop([p](){
            delete p;
        });
        puller_ = nullptr;
    }
}
