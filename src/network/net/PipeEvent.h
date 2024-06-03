/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-03 19:13:54
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 19:26:34
 * @FilePath: /tmms/src/network/net/PipeEvent.h
 * @Description:  learn 
 */
#pragma once
#include "Event.h"
#include <memory>

namespace tmms
{
    namespace network
    {
        class PipeEvent : public Event
        {
        public:
            using ptr = std::shared_ptr<PipeEvent>;

            PipeEvent(EventLoop *loop);
            ~PipeEvent();

        public:
            void OnRead() override;
            void OnClose() override;
            void OnError(const std::string &msg) override;

        public:
            void Write(const char *data, size_t len);
        
        private:
            int write_fd_{-1};
        };
    
        
    } // namespace network
    
} // namespace tmms
