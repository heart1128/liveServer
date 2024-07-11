/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 15:41:38
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-10 15:50:33
 * @FilePath: /liveServer/src/mmedia/mpegts/PmtWriter.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 * 
**/
#include "PmtWriter.h"
#include "mmedia/base/BytesWriter.h"

using namespace tmms::mm;

PmtWriter::PmtWriter()
{
    // pmt固定值
    table_id_ = 0x02;
    pid_ = 0x1001; 
}


/**
 * 
 * reserved 3 bslbf
 * PCR_PID 13 uimsbf
 * reserved 4 bslbf 
 * program_info_length 12 uimsbf
 * for(i=O;i<N;i++)
 *  descriptor()
 * for(i=0;i<N1;i++)
 * {
 *      stream_type 8 uimsbf
 *      reserved 3 bslbf
 *      elementary_PID 13 uimsbf
 *      reserved 4 bslbf
 *      ES_info_length 12 uimsbf
 *      for(i=0;i<N2;i++)
 *      descriptor()*
 * 
*/
void PmtWriter::Writepmt(StreamWriter *w)
{
    uint8_t section[kSectionMaxSize], *q;
    q = section;

    BytesWriter::WriteUint16T((char*)q, 0xe000 | pcr_pid_); // 低13位
    q += 2;
    BytesWriter::WriteUint16T((char*)q, 0xF000 | 0); // 高4位保留
    q += 2;
    for(auto const &p : programs_)
    {
        *q++ = p->stream_type;
        BytesWriter::WriteUint16T((char*)q, 0xe000 | p->elementary_pid); 
        q += 2;
        BytesWriter::WriteUint16T((char*)q, 0xF000 | 0); // 没有描述子
        q += 2;
    }
    PSIWriter::WriteSection(w, 0x0001, 0, 0, section, q - section);
}

// 添加节目
void PmtWriter::AddProgramInfo(ProgramInfoPtr &program)
{
    programs_.emplace_back(program);
}

void PmtWriter::SetPcrPid(int32_t pid)
{
    pcr_pid_ = pid;
}
