/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 15:11:54
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-10 15:18:33
 * @FilePath: /liveServer/src/mmedia/mpegts/PatWriter.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#include "PatWriter.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;


PatWriter::PatWriter()
{
    // 两个固定值
    pid_ = 0x0000;
    table_id_ = 0x00;
}

void PatWriter::WritePat(StreamWriter *w)
{
    uint8_t section[kSectionMaxSize], *q;
    q = section;
    // 从循环的那段section开始
    BytesWriter::WriteUint16T((char*)q, progoram_number_);  // progoram_number 16bit
    q += 2;
    BytesWriter::WriteUint16T((char*)q, 0xe000 | pmt_pid_); // 13 bit
    q += 2;

    // 组装成一个TS
    PSIWriter::WriteSection(w, transport_stream_id_, 0, 0, section, q - section);
}
