/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-12 16:09:36
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-12 16:19:23
 */
#include "BytesReader.h"
#include <netinet/in.h>
#include <cstring>
#include <cstdint>

using namespace tmms::mm;

uint64_t BytesReader::ReadUint64T(const char *data)
{
    uint64_t *c = (uint64_t*)data;
    return ntohl(*c);
}

uint32_t BytesReader::ReadUint32T(const char *data)
{
    uint32_t *c = (uint32_t*)data;
    return ntohl(*c);
}

/// @brief 
/// @param data 
/// @return 
uint32_t BytesReader::ReadUint24T(const char *data)
{
    unsigned char *c = (unsigned char*)data;
    uint32_t val;
    val = (c[0] << 16) | (c[0] << 8) | c[2];    // 组装24位3字节
    return val; // 已经是小端了
}

/// @brief 读取缓存头部的两个字节值
/// @param data 
/// @return 两个字节的short int值
uint16_t BytesReader::ReadUint16T(const char *data)
{
    uint16_t *c = (uint16_t*)data;
    return ntohs(*c);
}

/// @brief 读取缓存头部一个字节的值
/// @param data 网络数据缓存
/// @return 一个字节的值
uint8_t BytesReader::ReadUint8T(const char *data)
{
    return data[0];
}
