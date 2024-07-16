#include "FragmentWindow.h"
#include "mmedia/base/MMediaLog.h"
#include <sstream>
#include <cmath>

using namespace tmms::mm;

namespace 
{
    static FragmentPtr fragment_null;
} // namespace name


FragmentWindow::FragmentWindow(int32_t size)
:window_size_(size)
{
}

FragmentWindow::~FragmentWindow()
{
}

void FragmentWindow::AppendFragment(FragmentPtr &&fragment)
{
    {
        std::lock_guard<std::mutex> lk(lock_);
        fragments_.emplace_back(std::move(fragment));
    }

    Shrink();
    UpdatePlayList();
}

/// @brief 获取空闲的fragment，进行写入
/// @return 
FragmentPtr FragmentWindow::GetIdleFragment()
{
    std::lock_guard<std::mutex> lk(lock_);
    // 没有就申请一个
    if(free_fragments_.empty())
    {
        return std::make_shared<Fragment>();
    }
    else
    {
        auto p = free_fragments_[0];
        free_fragments_.erase(free_fragments_.begin());
        return p;
    }
}

/// @brief 通过gragment的文件名查找
/// @param name 
/// @return 
const FragmentPtr &FragmentWindow::GetFragmentByName(const std::string &name)
{
    std::lock_guard<std::mutex> lk(lock_);
    for(auto &f : fragments_)
    {
        if(f->FileName() == name)
        {
            return f;
        }
    }

    return fragment_null;
}

std::string FragmentWindow::GetPlayList()
{
    std::lock_guard<std::mutex> lk(lock_);
    return playlist_;
}

void FragmentWindow::Shrink()
{
    std::lock_guard<std::mutex> lk(lock_);
    int remove_index = -1;
    // 小于窗口值就不用删除
    if(fragments_.size() <= window_size_)
    {
        return;
    }

    remove_index = fragments_.size() - window_size_;
    // 一直从0删除到remove_index
    for(int i = 0; i < remove_index; ++i)
    {
        auto p = *fragments_.begin();
        fragments_.erase(fragments_.begin());
        p->Reset();
        free_fragments_.emplace_back(std::move(p));
    }
}

void FragmentWindow::UpdatePlayList()
{
    std::lock_guard<std::mutex> lk(lock_);
    if(fragments_.empty() || fragments_.size() < 3)
    {
        return;
    }

    std::ostringstream ss;
    ss << "#EXTM3U\n";           // 第一行必须是这个标志
    ss << "#EXT-X-VERSION:3 \n"; // 版本固定

    // 从窗口起始开始
    int i = fragments_.size() > 5 ? (fragments_.size() - 5) : 0;
    int j = i;
    int32_t max_duration = 0;
    // 求出最大时长，就是窗口内的片段最大时长
    for(; j < fragments_.size(); j++)
    {
       max_duration = std::max(max_duration, (int32_t)fragments_[j]->Duration());
    }

    int32_t target_duration = (int32_t)ceil((max_duration / 1000.0)); // 向上取整，变成s

    ss << "#EXT-X-TARGETDURATION:" << target_duration << "\n"; // s  // 表示每个媒体段最大的时长（秒），必写标签
    ss << "#EXT-X-MEDIA-SEQUENCE:" << fragments_[i]->SequenceNo() << "\n"; //表示播放列表第一个媒体段的URI的序列号，每个媒体片段的URI都拥有一个唯一的整型序列号
    ss.precision(3); // 设置输入的有效位数
    ss.setf(std::ios::fixed, std::ios::floatfield); // 设置固定精度，就是保留三位的float

    for(; i < fragments_.size(); ++i)
    {
        ss << "#EXTINF:" << fragments_[i]->Duration() / 1000.0 << "\n"; // 标记指定当前媒体段的持续时间，只作用于一个媒体段，就是时长
        ss << fragments_[i]->FileName() << "\n"; // 下一行要跟file
    }

    playlist_.clear();
    playlist_ = std::move(ss.str());
    HLS_TRACE << "playlist: \n" << playlist_;
}
