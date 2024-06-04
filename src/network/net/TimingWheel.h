#pragma once
#include <memory>
#include <functional>
#include <cstdint>
#include <vector>
#include <deque>
#include <unordered_set>

namespace tmms
{
    namespace network
    {
        using EntryPtr = std::shared_ptr<void>;
        using WheelEntry = std::unordered_set<EntryPtr>;
        using Wheel = std::deque<WheelEntry>;
        using Wheels = std::vector<Wheel>;     
        using Func = std::function<void()>;

        const int KTimingMinute = 60;
        const int KTimingHour = 60 * 60;
        const int KTimingDay = 60 * 60 * 24;

        enum TimingType
        {
            kTimingTypeSecond = 0,
            kTimingTypeMinute = 1,
            kTimingTypeHour = 2,
            kTimingTypeDay = 3,
        };

        class CallbackEntry
        {
        public:
            using ptr = std::shared_ptr<CallbackEntry>;
            CallbackEntry(const Func &cb): cb_(cb)
            {}
            ~CallbackEntry()
            {
                if(cb_)   // 从时间轮删除最后一个引用，析构进行回调
                {
                    cb_();
                }
            }        
        
        private:
            Func cb_;
        };

        class TimingWheel
        {
        public:
            using ptr = std::shared_ptr<TimingWheel>;
            TimingWheel();
            ~TimingWheel();
        
        public:
            void InstertEntry(uint32_t delay, EntryPtr entryPtr);   // 插入entry，设置超时时间
            void OnTimer(int64_t now);
            void PopUp(Wheel &bq);
            void RunAfter(double delay, const Func &cb);    // 设置延迟多少时间执行
            void RunAfter(double delay, const Func &&cb);
            void RunEvery(double interval, const Func &cb);// 每隔一段时间就执行一遍
            void RunEvery(double interval, const Func &&cb);

        private:
            void InstertSecondEntry(uint32_t delay, EntryPtr entryPtr);
            void InstertMinuteEntry(uint32_t delay, EntryPtr entryPtr);
            void InstertHourEntry(uint32_t delay, EntryPtr entryPtr);
            void InstertDayEntry(uint32_t delay, EntryPtr entryPtr);

            Wheels wheels_;
            int64_t last_ts_{0};
            uint64_t tick_{0};
        };
        
        
    } // namespace network
    
} // namespace tmms
