/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-09 13:36:37
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-09 13:36:40
 * @FilePath: /liveServer/src/mmedia/base/NalBitStream.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <cstdint>
namespace tmms
{
    namespace mm
    {
        /// @brief 按照比特流的方式读取数据
        class NalBitStream 
        {
        public:
            NalBitStream(const char *data, int len);
            uint8_t GetBit();
            uint16_t GetWord(int bits);
            uint32_t GetBitLong(int bits);
            uint64_t GetBit64(int bits);
            uint32_t GetUE();
            int32_t GetSE();
        private:
            char GetByte();
            const char * data_;
            int len_;
            int bits_count_;
            int byte_idx_;
            char byte_;
        };
    }
}