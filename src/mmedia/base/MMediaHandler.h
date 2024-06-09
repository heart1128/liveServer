/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 17:00:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 17:07:22
 * @FilePath: /tmms/src/mmedia/base/MMediaHandler.h
 * @Description:  learn 
 */
#pragma once
#include "base/NonCopyable.h"
#include "network/net/TcpConnection.h"
#include "Packet.h"
#include <memory>

/**
 *  代码中使用了大量回调函数，所以现在做一个基类统一管理公用的回调函数方便管理
 * 用基类指针就能调用，其他需要的回调可以继承增加
*/

namespace tmms
{
    namespace mm
    {
        using namespace network;

        class MMediaHandler : public NonCopyable
        {
        public:
            virtual void OnNewConnection(const TcpConnectionPtr &conn) = 0; // 新连接的时候，直播业务就可以处理数据，比如创建用户等
            virtual void OnConnectionDestroy(const TcpConnectionPtr &conn) = 0; // 连接断开的时候，业务层可以回收资源，注销用户等
            virtual void OnRecv(const TcpConnectionPtr& conn, const PacketPtr &data) = 0;   // 多媒体解析出来的数据，传给直播业务
            virtual void OnRecv(const TcpConnectionPtr& conn, PacketPtr &&data) = 0;
            virtual void OnActive(const ConnectionPtr &conn) = 0;   // 新连接回调通知直播业务
        };
        
    } // namespace mm
    
} // namespace name


