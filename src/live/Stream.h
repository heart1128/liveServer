/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 22:25:40
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-18 13:51:42
 * @FilePath: /liveServer/src/live/Stream.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-28 22:25:40
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-04 15:04:17
 * @FilePath: /liveServer/src/live/Stream.h
 * @Description:  learn 
 */
#pragma once

#include "live/base/TimeCorrector.h"
#include "live/base/GopMgr.h"
#include "live/base/CodecHeader.h"
#include "mmedia/base/Packet.h"
#include "live/user/PlayerUser.h"
#include "mmedia/hls/HLSMuxer.h"
#include "mmedia/mpegts/TsEncoder.h"
#include <string>
#include <memory>
#include <cstdint>
#include <atomic>
#include <vector>
#include <mutex>

namespace tmms
{
    namespace live
    {
        using namespace tmms::mm;

        class PlayerUser;
        using PlayerUserPtr = std::shared_ptr<PlayerUser>;
        class Session;

        class Stream
        {
        public:
            Stream(const std::string &session_name, Session &s);

        public:
        /// 时间处理函数
            int64_t ReadyTime() const ;
            int64_t SinceStart() const ;
            bool Timeout();
            int64_t DataTime() const ;
        /// 流信息的成员函数
            const std::string &SessionName() const ;
            int32_t StreamVersion() const;
            bool HasMedia() const;
            bool Ready() const;
        /// 输入成员函数（从推流中获取数据包）
            void AddPacket(PacketPtr&& packet);
        /// 输出成员函数
            void GetFrames(const PlayerUserPtr &user);

            bool HasVideo() const;
            bool HasAudio() const;
            std::string PlayList()
            {
                return muxer_.PlayList();
            }

            FragmentPtr GetFragment(const std::string &name)
            {
                return muxer_.GetFragment(name);
            }

        private:
            void SetReady(bool ready);

            bool LocateGop(const PlayerUserPtr &user);  // 没找到序列头就要定位gop
            void SkipFrame(const PlayerUserPtr &user);  //播放的时候可能要跳帧
            void GetNextFrame(const PlayerUserPtr &user);


        private:
            void ProcessHls(PacketPtr &packet);
            int64_t data_comming_time_{0}; // 第一个数据什么时候来的
            int64_t start_timestamp_{0};  // 创建流的时间
            int64_t ready_time_{0};         // 流准备好的时间
            std::atomic<int64_t> stream_time_{0}; // 当前流的时间，因为流线程和检测流超时的线程都在使用，所以用原子变量

            std::string session_name_; // 会话名
            Session &session_;      // 管理流和用户
            std::atomic<int64_t> frame_index_{-1}; // 当前流的帧号
            // 长度必须在vector前初始化，因为在构造函数用到了
            // 构造函数初始化的顺序就是声明的顺序
            uint32_t packet_buffer_size_{1000}; // 默认保存的packet是1000帧
            std::vector<PacketPtr> packet_buffer_;
            bool has_audio_{false};
            bool has_video_{false};
            bool has_meta_{false};
            bool ready_{false}; // 流有没有准备好
            std::atomic<int32_t> stream_version_{-1};

            GopMgr gop_mgr_;
            CodecHeader codec_headers_;
            TimeCorrector time_corrector_;
            std::mutex lock_;

            // TestStreamWriter write_;  // mpegts格式写入文件测试
            HLSMuxer muxer_;  // 在流不断的解析过程中，不断产生hls切片

        };  

    } // namespace live
    
} // namespace tmms
