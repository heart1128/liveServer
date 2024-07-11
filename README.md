<!--
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-02 17:00:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-11 16:51:32
 * @FilePath: /tmms/README.md
 * @Description:  learn 
-->
# 直播服务器
整个流程就是，启动liveserver，创建rtmpserver/httpserver监听，客户端连接之后有消息就回调上层，tcpconnection->sever->liveserver处理数据，进行解析，如果是rtmp就进行解析rtmp，如果是http就处理flv

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

## 10. FLV设计实现
flv不是一种协议，是一种数据封装格式，rtmp推流上服务器的数据可以用rtmp协议重新播放，也可以使用flv封装然后使用Http发送。
### 10.1 flvcontext
主要做的事情就是组装Http header + flv header + flv body([tag header, tag body])，然后使用tcpconnection发送。

### 10.2 flvPlayUser
实现控制发送头部请求等，通过flv播放。创建Player加入到session中，利用liveServer控制，机械能判断flv的使用和创建，flv实际使用的context是httpContext进行发送，在响应的时候也会使用flcContext进行组包,具体在HttpServer中。  
在通过tcpConnection->HttpContext->OnRequest回调到liveServer中，首先判断请求是flv还是rtmp，创建对应的playerUser，然后调用tcpConntcion的Avtive()，不断上层回调最终到liveServer的OnActive()中开始flv数据(数据从stream不断保存推流的数据拿)发送（之前注册的用户拿出来，rtmp用户用trmpPlayer，flv用FlvPlayer），首先FlvPlayerUser发送元数据，头信息，,遇到数据循环发送，然后利用flvContext使用tcp发送数据。

## 11. Demux实现
- flv格式 ： https://www.cnblogs.com/leisure_chn/p/10662941.html
- 直播推流推上来的音视频数据是通过FLV格式封装的，我们并没有解封装，因为我们实现的直播拉流用的协议是RTMP，刚好又需要FLV格式的封装。但是，接下来我们要实现的HLS协议，却是用的mpegts的封装，所以需要把FLV封装的音视频解封装，然后用mpegts重新封装，生成hls切片。另一方面，音视频在文件（FLV格式）存储和流式传输上存在差别:·音视频文件存储的方式把解码所需的信息放在文件的开始·流式传输需要在每一个单位（每一个TS）都插入解码所需的信息
- AVC分为：1. avcc 2. annexB格式

## 12. Mpegts实现
Mpegts是一种封装格式，和flv相同，作为传输的。
这里要实现的就是，从rtmp传输上来的数据是用flv封装的实时流，直播使用
而需要点播的就要把flv解包封装成MPEGTS结构，使用hls传输。
1. flv和MPEGTS的音频有两种
- AAC：两个格式不同，需要转换 flv:ADIF -> MPEGTS:ADTS
- MP3：格式相同，不需要转换
2. 视频不同，视频使用H.264（AVC）
- AVCC：是MPEG-4个格式,flv使用（在NALU前是长度）
- AnnexB：Mpeg-ts使用（在NALU前不是长度，是标志）