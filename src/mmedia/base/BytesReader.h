/*
 * @Description: BytesReader从缓存中取整形，从网络字节序转到主机字节序
 * @Version: 0.1
 * @Autor: heart
 * @Date: 2024-06-12 16:09:28
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-12 16:11:04
 */
#pragma once
#include <stdint.h>

namespace tmms
{
    namespace mm
    {
        class BytesReader
        {
        public:
            BytesReader() = default;
            ~BytesReader() = default;
            static uint64_t ReadUint64T(const char *data);
            static uint32_t ReadUint32T(const char *data);
            static uint32_t ReadUint24T(const char *data);
            static uint16_t ReadUint16T(const char *data);
            static uint8_t  ReadUint8T(const char *data);
        }; 
    }   
}

