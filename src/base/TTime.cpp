/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-05-31 23:09:30
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 14:50:26
 * @FilePath: /liveServer/src/base/TTime.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "TTime.h"

using namespace tmms::base;

/// @brief 获取当前的精确时间（1970.1.1到现在的），单位是ms
/// @return 精确时间，单位：ms
int64_t TTime::NowMS()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);                        // 获取精确时间到结构体
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;    // 一个是s,一个是us，换算成ms
}

/// @brief 获取当前的UTC时间
/// @return 返回数值，单位是s
int64_t TTime::Now()
{
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec;
}

/// @brief 获取年月日，时分秒
/// @param year 
/// @param month 
/// @param day 
/// @param hour 
/// @param minute 
/// @param second 
/// @return struct tm的时间结构体
int64_t TTime::Now(int &year, int &month, int &day, 
                                int &hour, int &minute, int &second)
{
    struct tm tm;
    time_t t = time(NULL);                          // 获取本地时间
    localtime_r(&t, &tm);                           // 本地时间转化到时间结构体中

    year = tm.tm_year + 1900;                       // 内部-1900了，所以要加1900
    month = tm.tm_mon + 1;                          // 内部是[0-11]的数字，所以+1
    day = tm.tm_mday;
    hour = tm.tm_hour;
    minute = tm.tm_min;
    second = tm.tm_sec;

    return t;
}

/// @brief 过去当前时间IOS字符串
/// @return 字符串的时间格式
std::string TTime::ISOTime()
{
    struct timeval tv;
    struct tm      tm;

    gettimeofday(&tv, NULL);
    time_t t = time(NULL);
    localtime_r(&t, &tm);

    char buf[128] = {0};
    auto n = sprintf(buf, "%4d-%02d-%02dT%02d:%02d:%02d",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec);
    return std::string(buf, buf + n);
}
