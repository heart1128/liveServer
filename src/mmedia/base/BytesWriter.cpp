/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 16:09:56
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-12 17:50:46
 */
#include "BytesWriter.h"
#include <netinet/in.h>
#include <cstring>

using namespace tmms::mm;

int BytesWriter::WriteUint32T(char *buf, uint32_t val)
{
    val = htonl(val);
    memcpy(buf, &val, sizeof(int32_t));
    return sizeof(int32_t);
}

int BytesWriter::WriteUint24T(char *buf, uint32_t val)
{
    val = htonl(val);
    char *ptr = (char*)&val;
    ptr++;  // 主机小端，网络大端
    memcpy(buf, ptr, 3);
    return 3;
}

int BytesWriter::WriteUint16T(char *buf, uint16_t val)
{
    val = htonl(val);
    memcpy(buf, &val, sizeof(int16_t));
    return sizeof(int16_t);
}

/// @brief 转换成网络字节序（大端）
/// @param buf 
/// @param val 
/// @return 转换的字节
int BytesWriter::WriteUint8T(char *buf, uint8_t val)
{
    char* data = (char*)&val;
    buf[0] = data[0];
    return 1;
}
