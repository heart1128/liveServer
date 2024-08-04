
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 21:34:54
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 15:17:45
 * @FilePath: /liveServer/src/mmedia/base/MMediaLog.h
 * @Description:  learn
 */
#pragma once
#include "base/LogStream.h"
#include <iostream>

using namespace tmms::base;

#define RTMP_DEBUG_ON 1
#define HTTP_DEBUG_ON 1
#define DEMUX_DEBUG_ON 1
#define MPEGTS_DEBUG_ON 1
#define HLS_DEBUG_ON 1
#define WEBRTC_DEBUG_ON 1 


// 这些可以不开,debug才开
#ifdef RTMP_DEBUG_ON
#define RTMP_TRACE LOG_TRACE << "RTMP::"
#define RTMP_DEBUG LOG_DEBUG << "RTMP::"
#define RTMP_INFO LOG_INFO << "RTMP::"
#else
#define RTMP_TRACE if(0) LOG_TRACE 
#define RTMP_DEBUG if(0) LOG_DEBUG 
#define RTMP_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define RTMP_WARN LOG_WARN
#define RTMP_ERROR LOG_ERROR

//// http

// 这些可以不开,debug才开
#ifdef HTTP_DEBUG_ON
#define HTTP_TRACE LOG_TRACE << "HTTP::"
#define HTTP_DEBUG LOG_DEBUG << "HTTP::"
#define HTTP_INFO LOG_INFO << "HTTP::"
#else
#define HTTP_TRACE if(0) LOG_TRACE 
#define HTTP_DEBUG if(0) LOG_DEBUG 
#define HTTP_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define HTTP_WARN LOG_WARN
#define HTTP_ERROR LOG_ERROR


//// demux

// 这些可以不开,debug才开
#ifdef DEMUX_DEBUG_ON
#define DEMUX_TRACE LOG_TRACE << "DEMUX::"
#define DEMUX_DEBUG LOG_DEBUG << "DEMUX::"
#define DEMUX_INFO LOG_INFO << "DEMUX::"
#else
#define DEMUX_TRACE if(0) LOG_TRACE 
#define DEMUX_DEBUG if(0) LOG_DEBUG 
#define DEMUX_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define DEMUX_WARN LOG_WARN
#define DEMUX_ERROR LOG_ERROR


//// mpegts
// 这些可以不开,debug才开
#ifdef MPEGTS_DEBUG_ON
#define MPEGTS_TRACE LOG_TRACE << "MPEGTS::"
#define MPEGTS_DEBUG LOG_DEBUG << "MPEGTS::"
#define MPEGTS_INFO LOG_INFO << "MPEGTS::"
#else
#define MPEGTS_TRACE if(0) LOG_TRACE 
#define MPEGTS_DEBUG if(0) LOG_DEBUG 
#define MPEGTS_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define MPEGTS_WARN LOG_WARN
#define MPEGTS_ERROR LOG_ERROR


//// HLS
// 这些可以不开,debug才开
#ifdef HLS_DEBUG_ON
#define HLS_TRACE LOG_TRACE << "HLS::"
#define HLS_DEBUG LOG_DEBUG << "HLS::"
#define HLS_INFO LOG_INFO << "HLS::"
#else
#define HLS_TRACE if(0) LOG_TRACE 
#define HLS_DEBUG if(0) LOG_DEBUG 
#define HLS_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define HLS_WARN LOG_WARN
#define HLS_ERROR LOG_ERROR


//// WebRtc
// 这些可以不开,debug才开
#ifdef WEBRTC_DEBUG_ON
#define WEBRTC_TRACE LOG_TRACE << "WEBRTC::"
#define WEBRTC_DEBUG LOG_DEBUG << "WEBRTC::"
#define WEBRTC_INFO LOG_INFO << "WEBRTC::"
#else
#define WEBRTC_TRACE if(0) LOG_TRACE 
#define WEBRTC_DEBUG if(0) LOG_DEBUG 
#define WEBRTC_INFO if(0) LOG_INFO 
#endif

// 出错和警告必须开
#define WEBRTC_WARN LOG_WARN
#define WEBRTC_ERROR LOG_ERROR