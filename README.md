<!--
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 17:00:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-24 21:45:40
 * @FilePath: /tmms/README.md
 * @Description:  learn 
-->
# 直播服务器

## 1. base库
## 2. 网络服务器
### 2.1 
### 2.2
### 2.3
### 2.4
### 2.5 时间轮
用vector模拟时间轮，每一个槽都是一个deque装着相同到时时间的任务，每个任务都是智能指针包裹，当时间轮内的所有引用全部消失，进行自动析构，析构内进行回调。
如果要增加时间，智能指针会加引用，不会析构引起回调。

## 8. 直播业务管理
## 8.1 直播配置解析
用了domain配置解析和app配置，其中一个domain包含多个app(rtmp://domain/app_xxx)
## 8.2 时间戳校正
1. 当推流的时候中断了，重新推流，时间戳变为0。不能让用户感知到这个问题，需要修正时间戳。
2. 实时音视频输出，如果用户端卡了，不能让帧停留在那里，直播实时的需要跳帧，改时间戳。（使用视频的时间戳为准）