/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 16:09:45
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-12 16:19:58
 */
#pragma once
#include <stdint.h>

namespace tmms
{
    namespace mm
    {
        class BytesWriter
        {
        public:
            BytesWriter() = default;
            ~BytesWriter() = default;
            
            static int WriteUint32T(char *buf, uint32_t val);
            static int WriteUint24T(char *buf, uint32_t val);
            static int WriteUint16T(char *buf, uint16_t val);
            static int WriteUint8T(char *buf, uint8_t val);
        };  
    }  
}
