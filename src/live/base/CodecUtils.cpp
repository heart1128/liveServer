/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-24 22:57:34
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-28 23:08:37
 * @FilePath: /liveServer/src/live/base/CodecUtils.cpp
 * @Description:  learn 
 */
#include "CodecUtils.h"

using namespace tmms::live; 

/*
1. FLV Tag Header
+-----------------+------------------+
| PreviousTagSize |     4 bytes      | 前一个标签的大小（4字节）。
+-----------------+------------------+
|     TagType     |     1 byte       | 标签类型（1字节），0x08 表示音频，0x09 表示视频，0x12 表示脚本数据。
+-----------------+------------------+
|    DataSize     |     3 bytes      | 数据大小（3字节），表示标签体的大小。
+-----------------+------------------+
|    Timestamp    |     3 bytes      |
+-----------------+------------------+
|TimestampExtended|     1 byte       |
+-----------------+------------------+
|     StreamID    |     3 bytes      |
+-----------------+------------------+

2. FLV Tag Body (视频数据)
a. AVC Sequence Header
+-------------------------+------------------+
| FrameType & CodecID     |     1 byte       |  FrameType: 高四位表示帧类型，1表示关键帧，2表示内编码帧 低四位表示编码ID，7表示AVC（H.264）
+-------------------------+------------------+
|   AVCPacketType         |     1 byte       | AVC包类型（1字节）。0 表示AVC sequence header（pps等）。 1 表示AVC NALU。（实际音视频数据） 2 表示AVC end of sequence。
+-------------------------+------------------+
| CompositionTime         |     3 bytes      |  合成时间（3字节）。
+-------------------------+------------------+
|   AVCDecoderConfigurationRecord           |  包含SPS, PPS和其他配置数据。
| (SPS, PPS, and other configuration data)  |
+-------------------------+------------------+

*/

bool CodecUtils::IsCodecHeader(const PacketPtr &packet)
{
    // 第一个字节是FLV
    if(packet->PacketSize() > 1)
    {
        const char *b = packet->Data() + 1;
        // 音频流的AAC第二个字节为0 
        // 视频流的第二个字节为0，就是H.264的
        if(*b == 0)
        {
            return true;
        }
    }
    return false;
}

/// @brief 判断是不是关键字，原理就是rtmp的数据是flv封装的
/// @param packet 
/// @return 
bool CodecUtils::IsKeyFrame(const PacketPtr &packet)
{
    // 检查包的大小是否大于0
    if (packet->PacketSize() > 0)
    {
        // 获取数据的第一个字节
        const char *b = packet->Data();
    
        // FLV视频标签的第一个字节的高四位表示帧类型
        // 1表示关键帧，2表示内编码帧，3表示预测帧等
        // 检查是否为关键帧
        return ((*b >> 4) & 0x0F) == 1;
    }
    return false;
}
