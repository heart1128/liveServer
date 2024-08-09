/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-08 21:07:52
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-08 21:08:06
 * @FilePath: /liveServer/src/mmedia/webrtc/DtlsHandler.h
 * @Description:  learn 
 * 
 */
#pragma once

#include <cstdint>
#include <cstddef>

namespace tmms
{
    namespace mm
    {
        class Dtls;
        class DtlsHandler
        {
        public:
            virtual void OnDtlsSend(const char *data, size_t size, Dtls *dtls){}
            virtual void OnDtlsHandshakeDone(Dtls *dtls){}
        };
        
        
    } // namespace mm
    
} // namespace tmms

