/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-17 17:26:34
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-17 17:36:55
 * @FilePath: /liveServer/src/live/relay/PullerRelay.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AEname
**/
#pragma once

#include "live/relay/pull/Puller.h"
#include "live/Session.h"
#include <vector>

namespace tmms
{   
    namespace live
    {
        class PullerRelay : public PullHandler
        {
        public:
            PullerRelay(Session &s);
            ~PullerRelay();

            void StartPullStream(); // 回源入口

        
        private:
            void OnPullSucess() override;
            void OnPullClose() override;

            bool GetTargets();
            Puller *GetPuller(TargetPtr p);
            void SelectTarget();
            void Pull();
            void ClearPuller();
        
        private:
            Session &session_; // 成员变量可以声明引用，不过必须在构造函数初始化列表中
            std::vector<TargetPtr> targets_;      // 读取配置多个target
            TargetPtr current_target_;          // 回源到哪个target了
            int32_t cur_target_index_{-1};
            Puller * puller_{nullptr};
            // fixbug:没有初始化为空，后面判断一直不为空就不创建对象了
            EventLoop *current_loop_{nullptr};
        };
        
        
    } // namespace live
    
} // namespace tmms
    