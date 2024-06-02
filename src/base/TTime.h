/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-05-31 23:03:10
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-05-31 23:10:03
 * @FilePath: /liveServer/src/base/TTime.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AETIME
 */
#pragma once

#include <cstdint>
#include <string>
#include <sys/time.h>
#include <unistd.h>

namespace tmms
{
    namespace base
    {
        class TTime
        {
        public:
            static int64_t NowMS();
            static int64_t Now();
            static int64_t Now(int &year, int &month, int &day, 
                                int &hour, int & minute, int &second);
            static std::string ISOTime();
        };
    }
}