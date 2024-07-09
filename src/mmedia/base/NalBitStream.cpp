/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-09 13:37:15
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-09 13:37:18
 * @FilePath: /liveServer/src/mmedia/base/NalBitStream.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "NalBitStream.h"

using namespace tmms::mm;

NalBitStream::NalBitStream(const char *p, int len) 
: data_(p),len_(len),bits_count_(0),byte_idx_(0),byte_(0)
{

}

uint8_t NalBitStream::GetBit() {
    if (bits_count_ == 0) 
    {
        byte_ = GetByte();
        bits_count_ = 8;
    }
    bits_count_--;
    return (byte_>>bits_count_) & 0x01;
}

uint16_t NalBitStream::GetWord(int bits) {
    uint16_t u = 0;
    while(bits > 0) 
    {
        u <<= 1;
        u |= GetBit();
        bits--;
    }
    return u;
}
uint32_t NalBitStream::GetBitLong(int n)
{
    if (!n) 
    {
        return 0;
    } 
    else if (n <= 16) 
    {
        return GetWord(n);
    } 
    else 
    {
        unsigned ret = GetWord(16) << (n - 16);
        return ret | GetWord(n - 16);
    }
}
uint64_t NalBitStream::GetBit64(int n)
{
    if (n <= 32) 
    {
        return GetBitLong(n);
    } 
    else 
    {
        uint64_t ret = (uint64_t) GetBitLong(n - 32) << 32;
        return ret | GetBitLong(32);
    }
}
uint32_t NalBitStream::GetUE() 
{
    int zeros = 0;
    while(byte_idx_ < len_ && GetBit() == 0) 
    {
        zeros++;
    }
    return GetWord(zeros) + ((1 << zeros) - 1);
}

int32_t NalBitStream::GetSE() 
{
    uint32_t ue = GetUE();
    bool positive = ue & 1;
    int32_t se = (ue + 1) >> 1;
    if (!positive) 
    {
        se = -se;
    }
    return se;
}

char NalBitStream::GetByte() 
{
    if (byte_idx_ >= len_) 
    {
        return 0;
    }

    if (data_[byte_idx_] == 0x03) 
    {
        if (byte_idx_ >= 2) 
        {
            if (data_[byte_idx_ - 1] == 0 && data_[byte_idx_ - 2] == 0) 
            { 
                byte_idx_++;
            }
        }
    }
    return data_[byte_idx_++];
}