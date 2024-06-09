/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 16:30:36
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 17:06:36
 * @FilePath: /tmms/src/mmedia/base/Packet.h
 * @Description:  learn 
 */
#pragma once

#include <string>
#include <memory>
#include <cstring>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        enum
        {
            /// @brief 用二进制位表示，方便进行&操作，可能同时判断两种类型
            kPacketTypeVideo = 1,
            kPacketTypeAudio = 2,
            kPacketTypeMeta = 4,
            kPacketTypeMeta3 = 8,
            kPacketTypeKeyFrame = 16,
            kPacketTypeIDR = 32,
            kPacketTypeUnKnowed = 255,
        };

        /**
         * 
         * packet 保存音视频的packet，分配内存等
         * 
        */
        using PacketPtr = std::shared_ptr<Packet>;
#pragma pack(push)  // 保存字节对齐状态
#pragma pack(1)     // 因为编译器会自动对齐类字节，所以这里设置成1字节对齐节省空间
        class Packet
        {
        public:
            using ptr = std::shared_ptr<Packet>;

            Packet(int32_t size)
            :size_(size)
            {};
            ~Packet(){};
        
        public:
            static Packet::ptr NewPacket(int32_t size);

            bool IsVideo() const
            {
                return (type_ & kPacketTypeVideo) == kPacketTypeVideo;
            } 
            bool IsAudio() const
            {
                // 音频没有其他属性，直接判断
                return type_ == kPacketTypeAudio;
            }
            bool IsKeyFrame() const
            {
                // 只有视频有关键字
                return ((type_ & kPacketTypeVideo) == kPacketTypeVideo &&
                        (type_ & kPacketTypeKeyFrame) == kPacketTypeKeyFrame);
            }
            bool IsMeta() const
            {
                return type_ == kPacketTypeMeta;
            }
            bool IsMeta3() const
            {
                return type_ == kPacketTypeMeta3;
            }

            int32_t PakcetSize() const
            {
                return size_;
            }
            int Space() const              // 剩余空间
            {
                return capacity_ - size_;
            }
            void SetPacketSize(size_t len)
            {
                size_ = len;
            }
            void UpdatePacketSize(size_t len)
            {
                size_ += len;
            }
            void SetIndex(int32_t index)
            {
                index_ = index;
            }
            int32_t Index() const
            {
                return index_;
            }
            void SetPacketType(int32_t type)
            {
                type_ = type;
            }
            int32_t PacketType() const
            {
                return type_;
            }
            void SetTimeStamp(uint64_t timestamp)
            {
                timestamp_ = timestamp;
            }
            uint64_t TimeStamp() const
            {
                return timestamp_;
            }
            inline char *Data()
            {
                // 数据包是包含类的，this是开头，一直到类大小是数据的开头
                return (char*)this + sizeof(Packet);
            }

            template <typename T>
            std::shared_ptr<T> Ext() const;
            void SetExt(const std::shared_ptr<void> &ext)
            {
                ext_ = ext;
            }

        private:
            int32_t type_{kPacketTypeUnKnowed};
            uint32_t size_{0};          // 大小
            int32_t index_{-1};         // 读取的下标
            uint64_t timestamp_{0};     // 时间戳
            uint32_t capacity_{0};      // packet的容量
            std::shared_ptr<void> ext_; // 额外数据的指针
        };

        template <typename T>
        inline std::shared_ptr<T> Packet::Ext() const
        {
            return std::static_pointer_cast<T>(ext_);
        }
#pragma pack()  // 恢复对齐方式
    } // namespace mm

} // namespace tmms
