/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 16:00:52
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 16:24:01
 * @FilePath: /tmms/src/network/TestContext.cpp
 * @Description:  learn 
 */
#include "TestContext.h"
#include <iostream>

using namespace tmms::network;

TestContext::TestContext(const TcpConnectionPtr &con)
:connection_(con)
{
}

/// @brief 状态机解析消息
/// @param buf 
/// @return 返回1表示需要更多的数据，当前的数据不够；0成功；-1出错
int TestContext::ParseMessage(MsgBuffer &buf)
{
    while(buf.ReadableBytes() > 0)
    {
        if(state_ == kTestContextHeader)
        {
            if(buf.ReadableBytes() >= 4)    // 长度，用的int32，最少4字节
            {
                message_length_ = buf.ReadInt32();
                std::cout << "message_length_ : " << message_length_ << std::endl;
                state_ = kTestContextBody;
            }
            else
            {
                return 1;      
            }
        }
        else if(state_ == kTestContextBody)
        {
            // 1. 可读数据量大于长度，那就是后面部分是下一个包的长度，只需要读这部分包的长度就行
            if(buf.ReadableBytes() >= message_length_)
            {
                std::string tmp;
                // peek()返回单当前读的指针，直接给长度
                tmp.assign(buf.Peek(), message_length_);
                message_body_.append(tmp);
                buf.Retrieve(message_length_);  // 回缩buf
                message_length_ = 0;

                if(cb_)
                {
                    cb_(connection_, message_body_);
                    message_body_.clear();
                }
                state_ = kTestContextHeader;
            }
        }
    }
    return 0;
}

void TestContext::SetTestMessageCallback(const TestMessageCallback &cb)
{
    cb_ = cb;
}

void TestContext::SetTestMessageCallback(TestMessageCallback &&cb)
{
    cb_ = std::move(cb);
}
