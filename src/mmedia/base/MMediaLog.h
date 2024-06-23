/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 21:34:54
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-23 13:59:31
 * @FilePath: /tmms/src/mmedia/base/MMediaLog.h
 * @Description:  learn
 */
#pragma once
#include "base/LogStream.h"
#include <iostream>

using namespace tmms::base;

#define RTMP_DEBUG_ON 1

// 这些可以不开,debug才开
#ifdef RTMP_DEBUG_ON
#define RTMP_TRACE std::cout << "\n"
#define RTMP_DEBUG LOG_DEBUG 
#define RTMP_INFO LOG_INFO 
#elif
#define RTMP_TRACE if(0) LOG_TRACE 
#define RTMP_DEBUG if(0) LOG_DEBUG 
#define RTMP_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define RTMP_WARN LOG_WARN
#define RTMP_ERROR LOG_ERROR