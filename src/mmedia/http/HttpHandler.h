/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-02 15:29:32
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-04 16:10:57
 * @FilePath: /liveServer/src/mmedia/http/HttpHandler.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AEba
 */
#pragma once

#include "mmedia/base/MMediaHandler.h"

// 设置回调函数接口
namespace tmms
{
    namespace mm
    {
        class HttpRequest;
        using HttpRequestPtr = std::shared_ptr<HttpRequest>;
        
        class HttpHandler: virtual public MMediaHandler
        {
        public:
            virtual void OnSent(const TcpConnectionPtr &conn) = 0;
            virtual bool OnSentNextChunk(const TcpConnectionPtr &conn) = 0;   
            virtual void OnRequest(const TcpConnectionPtr &conn,const HttpRequestPtr &req,const PacketPtr &packet) = 0; 
        }; 

    } // namespace mm
    
} // namespace tmms
