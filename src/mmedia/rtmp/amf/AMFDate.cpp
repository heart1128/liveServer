/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:28:32
 */
#include "AMFDate.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "AMFDate.h"

using namespace tmms::mm;

AMFDate::AMFDate(const std::string &name)
: AMFAny(name)
{
}

AMFDate::AMFDate()
{
}

AMFDate::~AMFDate()
{
}

int AMFDate::Decode(const char *data, int size, bool has)
{
    // 够不够解析长度
    // Date类型 = 0x0b + DOUBLE(8bytes) + time-zene(2bytes)
    if(size < 10)
    {
        return -1;
    }

    utc_ = BytesReader::ReadUint64T(data);
    data += 8;
    utc_offset_ = BytesReader::ReadUint16T(data);
    return 10;
}

bool AMFDate::IsDate()
{
    return true;  // 是Date类，实现父类的判断函数，动态绑定就能执指向
}

double AMFDate::Date()
{
    return utc_;
}

void AMFDate::Dump() const
{
    RTMP_TRACE << "Date: " << utc_;
}

int16_t AMFDate::UtcOffset() const
{
    return utc_offset_;
}