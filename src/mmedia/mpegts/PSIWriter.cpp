#include "PSIWriter.h"
#include "mmedia/base/BytesWriter.h"
#include "TsTool.h"
#include <cstring>

using namespace tmms::mm;

void PSIWriter::SetVersion(uint8_t v)
{
    version_ = v;
}


/**
 * @description: PSE曾的信息，多ES封装，Section是Table的分解
 * @param {StreamWriter} *w
 * @param {int} id
 * @param {int} sec_num
 * @param {int} last_sec_num
 * @param {uint8_t} *buf
 * @param {int} len
 * @return {*}
**/
int PSIWriter::WriteSection(StreamWriter *w, int id, int sec_num, int last_sec_num, uint8_t *buf, int len)
{
    uint8_t section[1024], *p;
    auto total_len = len + 3 + 5 + 4; // 3 : table_id - section_length , 5 : tr_s_id - lasts_section_num;  4 ：crc
    // 组装PSE结构
    p = section;
    *p++ = table_id_;
    BytesWriter::WriteUint16T((char*)p, (5 + 4 + len) | 0xb000);
    p += 2;
    BytesWriter::WriteUint16T((char*)p, id); // transport_stream_id
    p += 2;

    *p++ = 0xc1 | (version_ << 1); // version 5 位
    *p++ = sec_num;
    *p++ = last_sec_num;

    memcpy(p, buf, len); // 写入回传的buf
    p += len;

    // 计算crc(ts表前面字段，但是不包括crc的4字节)
    auto crc = TsTool::CRC32(section, total_len - 4);
    BytesWriter::WriteUint32T((char*)p, crc);
    PushSection(w, section, total_len);
    return 0;
}

/**
 * @description: TS packet
 * @param {StreamWriter} *w
 * @param {uint8_t} *buf
 * @param {int} len
 * @return {*}
**/
void PSIWriter::PushSection(StreamWriter *w, uint8_t *buf, int len)
{
    // 固定188字节
    uint8_t packet[188], *q;
    uint8_t* p = buf;
    bool first = false; // 判断是不是第一个包

    while(len > 0)
    {
        q = packet;
        first = (p == buf);
        *q++ = 0x47; // 前面的一个字节
        auto b = pid_ >> 8; // PID
        if(first) // 第一个包附加信息
        {
            b |= 0x40;
        }
        *q++ = b;
        *q++ = pid_;
        cc_ = (cc_ + 1) & 0xf; // counter
        *q++ = 0x10 | cc_;  // 设置为10，表示只有纯负载
        if(first) // 第一个包要设置偏移量，但是这个值没意义
        {
            *q++ = 0;
        }

        // 保证剩余空间够
        auto len1 = 188 - (q - packet);
        if(len1 > len)
        {
            len1 = len;
        }
        memcpy(q, p , len1);
        q += len1;

        // TS包不足188字节，需要填充无效信息
        auto left = 188 - (q - packet);
        if(left > 0)
        {
            memset(q, 0xff, left);
        }

        w->Write(packet, 188);
        p += len1;
        len -= len1;
    }
}
