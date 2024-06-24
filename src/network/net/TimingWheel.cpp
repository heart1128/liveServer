#include "TimingWheel.h"
#include "network/base/Network.h"

using namespace tmms::network;

TimingWheel::TimingWheel()
:wheels_(4) // 会直接调用vector的构造函数
{
    wheels_[kTimingTypeSecond].resize(60);      // 一个deque的槽就是一个时间点，保存很多任务
    wheels_[kTimingTypeMinute].resize(60);
    wheels_[kTimingTypeHour].resize(24);
    wheels_[kTimingTypeDay].resize(30);
}

TimingWheel::~TimingWheel()
{
}

/// @brief  判断插入的时间，循环调用的方式最后执行任务插入到秒的时间轮
/// @param delay 
/// @param entryPtr 
void TimingWheel::InstertEntry(uint32_t delay, EntryPtr entryPtr)
{
    if(delay <= 0)      // 立即执行的函数
    {
        entryPtr.reset();   // 清空引用，自动析构，执行回调
    }
    else if(delay < KTimingMinute)  // 小于1分钟，是秒
    {
        InstertSecondEntry(delay, entryPtr);
    }
    else if(delay < KTimingHour) // 小于1小时，是分钟
    {
        InstertMinuteEntry(delay, entryPtr);
    }
    else if(delay < KTimingDay) // 小于1天，是小时 
    {
        InstertHourEntry(delay, entryPtr);
    }
    else
    {
        auto day = delay / KTimingDay;
        if(day > 30)        // 不支持超过30天的时间
        {
            NETWORK_ERROR << "It is not surpport > 30 day!!!";
            return;
        }
        InstertDayEntry(delay, entryPtr);
    }
}

/// @brief  转动时间轮，记录转动的秒数判断是否执行分，时，天的时间轮
/// @param now 
void TimingWheel::OnTimer(int64_t now)
{
    if(last_ts_ == 0)
    {
        last_ts_ = now;
    }

    if(now - last_ts_ < 1000)   // 1s时间执行一次秒的时间轮
    {
        return;
    }

    last_ts_ = now; // 上次执行的时间更新
    ++tick_;    // 执行次数
    PopUp(wheels_[kTimingTypeSecond]);

    if(tick_ % KTimingMinute == 0)  // 转动了60s的倍数，也就是分钟，处理分钟
    {
        PopUp(wheels_[kTimingTypeMinute]);
    }
    else if(tick_ % KTimingHour == 0)
    {
        PopUp(wheels_[kTimingTypeHour]);
    }
    else if(tick_ % KTimingDay == 0)
    {
        PopUp(wheels_[kTimingTypeDay]);
    }
}

void TimingWheel::PopUp(Wheel & bq)
{
    WheelEntry tmp;
    // bq.front().clear(); // bug: clear()清理元素但是不调用析构 
    bq.front().swap(tmp);   // 这样会调用析构，总是deque的头部先到期的，清理头部，头部是unordered_set
    bq.pop_front();     // 弹出前面的set
    bq.push_back(WheelEntry()); // 插一个新的槽，保证槽的数量不变
}

void TimingWheel::RunAfter(double delay, const Func &cb)
{
    CallbackEntry::ptr cbEntry = std::make_shared<CallbackEntry>([cb](){
        cb();
    });
    // 插入，到期执行
    InstertEntry(delay, cbEntry);
}

void TimingWheel::RunAfter(double delay, const Func &&cb)
{
    CallbackEntry::ptr cbEntry = std::make_shared<CallbackEntry>([cb](){
        cb();
    });
    // 插入，到期执行
    InstertEntry(delay, cbEntry);
}

void TimingWheel::RunEvery(double interval, const Func &cb)
{
    CallbackEntry::ptr cbEntry = std::make_shared<CallbackEntry>([this, cb, interval](){
        cb();
        RunEvery(interval, cb);  // 这里的技巧就是连环调用，执行函数之后，继续调用自己
    });
    // 插入，到期执行
    InstertEntry(interval, cbEntry);
}

void TimingWheel::RunEvery(double interval, const Func &&cb)
{
    CallbackEntry::ptr cbEntry = std::make_shared<CallbackEntry>([this, cb, interval](){
        cb();
        RunEvery(interval, cb);  // 这里的技巧就是连环调用，执行函数之后，继续调用自己
    });
    // 插入，到期执行
    InstertEntry(interval, cbEntry);
}


void TimingWheel::InstertSecondEntry(uint32_t delay, EntryPtr entryPtr)
{
    // delay - 1因为下标从0开始
    wheels_[kTimingTypeSecond][delay - 1].emplace(entryPtr);
}

/// @brief 设置的时间超过一分钟，不超过一小时的槽
/// @param delay 
/// @param entryPtr 
void TimingWheel::InstertMinuteEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto minute = delay / KTimingMinute;
    auto second = delay % KTimingMinute;
    // 回调函数执行秒的插入，也就是说，比如定时到12:01:10
    // 这里设置了插入秒作为插入分的回调函数，时间到12:01:00就触发分钟的槽执行回调函数，插入秒。
    // 然后秒判断到了12:01:10就执行真正的回调函数任务，所以秒的槽才是真正执行回调函数的地方
    CallbackEntry::ptr newEntryPtr = std::make_shared<CallbackEntry>([this, entryPtr, second](){
        InstertEntry(second, entryPtr);
    });

    wheels_[kTimingTypeMinute][minute - 1].emplace(newEntryPtr);
}

void tmms::network::TimingWheel::InstertHourEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto hour = delay / KTimingHour;
    auto second = delay % KTimingHour;

    CallbackEntry::ptr newEntryPtr = std::make_shared<CallbackEntry>([this, entryPtr, second](){
        InstertEntry(second, entryPtr);
    });

    wheels_[kTimingTypeHour][hour - 1].emplace(newEntryPtr);
}

void tmms::network::TimingWheel::InstertDayEntry(uint32_t delay, EntryPtr entryPtr)
{
    auto day = delay / KTimingDay;
    auto second = delay % KTimingDay;

    CallbackEntry::ptr newEntryPtr = std::make_shared<CallbackEntry>([this, entryPtr, second](){
        InstertEntry(second, entryPtr);
    });

    wheels_[kTimingTypeDay][day - 1].emplace(newEntryPtr);
}
