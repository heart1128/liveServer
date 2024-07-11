/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 10:40:01
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-10 15:13:36
 * @FilePath: /liveServer/src/mmedia/mpegts/PSIWriter.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include <cstdint>
#include "StreamWriter.h"

namespace tmms
{
    namespace mm
    {
        /**
         * @description: PSI是ES/table层的，有四种PSI,分别继承实现
        **/        
        class PSIWriter
        {
        public:
            PSIWriter() = default;
            virtual ~PSIWriter() = default;


            void SetVersion(uint8_t v);
            int WriteSection(StreamWriter * w, int id, int sec_num, int last_sec_num,uint8_t *buf, int len);
        
        protected:
            void PushSection(StreamWriter * w,uint8_t *buf, int len);

            int8_t cc_{-1};     // 计数
            uint16_t pid_{0xe000};   // ts包的唯一标识 13 位
            uint8_t table_id_{0x00};
            int8_t version_{0x00};
        };
        
    } // namespace mm
    
} // namespace tmms
