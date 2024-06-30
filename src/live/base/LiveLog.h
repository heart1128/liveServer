/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 21:34:54
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 22:26:14
 * @FilePath: /liveServer/src/live/base/LiveLog.h
 * @Description:  learn
 */
#pragma once
#include "base/LogStream.h"
#include <iostream>

using namespace tmms::base;

#define LIVE_DEBUG_ON 1

// 这些可以不开,debug才开
#ifdef LIVE_DEBUG_ON
#define LIVE_TRACE LOG_TRACE << "LIVE::"
#define LIVE_DEBUG LOG_DEBUG << "LIVE::"
#define LIVE_INFO LOG_INFO << "LIVE::"
#elif
#define LIVE_TRACE if(0) LOG_TRACE 
#define LIVE_DEBUG if(0) LOG_DEBUG 
#define LIVE_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define LIVE_WARN LOG_WARN
#define LIVE_ERROR LOG_ERROR