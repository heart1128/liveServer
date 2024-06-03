#pragma once

#include <string>
#include <memory>
#include <sys/epoll.h>

namespace tmms
{
    namespace network
    {  
        class EventLoop;    // 避免交叉引用，用前向声明，也就是有其他文件同时引用eventloop和event就出现问题
        const int kEventRead = (EPOLLIN | EPOLLPRI | EPOLLET);  // 方便调用操作,读，紧急，边沿触发事件
        const int kEventWrite = (EPOLLOUT | EPOLLET);

        class Event : public std::enable_shared_from_this<Event>    // 作为基类使用
        {
            friend class EventLoop;
        public:
            using ptr = std::shared_ptr<Event>;

            Event();
            Event(EventLoop *loop);
            Event(EventLoop *loop, int fd);
            ~Event();
        
        public:
            virtual void OnRead();
            virtual void OnWrite();
            virtual void OnClose();
            virtual void OnError(const std::string &msg);
            
        public:
            bool EnableWriting(bool enable);
            bool EnableReading(bool enable);
            int Fd() const;
        
        protected:
            int fd_{-1};
            EventLoop *loop_{nullptr}; // 在哪个loop中
            int event_{0};         // 什么事件 EPOLLIN，一定要初始化成0，初始化为-1就出错了
        }; 
    } 
}
