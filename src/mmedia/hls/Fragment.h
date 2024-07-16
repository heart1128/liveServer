/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-16 15:38:46
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-16 16:00:12
 * @FilePath: /liveServer/src/mmedia/hls/Fragment.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "mmedia/base/Packet.h"
#include "mmedia/mpegts/StreamWriter.h"

#include <string>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        const int32_t kFragmentStepSize = 128 * 1024; // 默认每一段使用内存进行缓存，如果缓存不够就进行扩容，这是每次扩容大小

        /// @brief 继承StreamWriter，实现接口，后续处理的mepgts数据都从这里送出, HLS就是每个切片组成的，使用http发送切片，每个切片都有ts能单独播放
        class Fragment : public StreamWriter
        {
        public:
            Fragment() = default;
            ~Fragment() = default;

            void AppendTimeStamp(int64_t pts) override;
            int32_t Write(void* buf, uint32_t size) override;
            char* Data() override;
            int Size() override;

            int64_t Duration() const;
            const std::string &FileName() const;
            void SetBaseFileName(const std::string &v);
            int32_t SequenceNo() const;
            void SetSequenceNo(int32_t no);
            void Reset();
            PacketPtr &FragmentData();
            void Save();
        
        private:
            int64_t duration_{0};
            std::string filename_;
            int64_t start_dts_{-1};     // 开始时间戳
            PacketPtr data_;
            int32_t buf_size_{512 * 1024}; // 512k
            int32_t data_size_{0};
            int32_t sequence_no_{0};
        };
        
        
    } // namespace mm
    
} // namespace tmms
