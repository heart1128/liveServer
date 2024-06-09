/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 15:55:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 16:08:31
 * @FilePath: /tmms/src/network/TestContext.h
 * @Description:  learn 
 */
#pragma once
#include "network/net/TcpConnection.h"
#include <string>
#include <functional>
#include <memory>

namespace tmms
{
    namespace network
    {
        // 回调到哪个tcpconnect, 解析的消息
        using TestMessageCallback = std::function<void(const TcpConnectionPtr&, const std::string&)>;

        class TestContext
        {
            enum
            {
                kTestContextHeader = 1,
                kTestContextBody = 2,
            };

        public:
            TestContext(const TcpConnectionPtr &con);
            ~TestContext() = default;

        public:
            int ParseMessage(MsgBuffer &buf);
            void SetTestMessageCallback(const TestMessageCallback &cb);
            void SetTestMessageCallback(TestMessageCallback &&cb);

        private:
            TcpConnectionPtr connection_;   // 解析之后要给connection
            int state_{kTestContextHeader}; // 状态机
            int32_t message_length_{0};     // 消息长度
            std::string message_body_;      // 消息体
            TestMessageCallback cb_;
        };
        
        
        
    } // namespace network
    
} // namespace tmms
