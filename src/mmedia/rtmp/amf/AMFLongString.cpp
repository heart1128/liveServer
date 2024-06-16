/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:28:58
 */
#include "AMFLongString.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

AMFLongString::AMFLongString(const std::string &name)
: AMFAny(name)
{
}

AMFLongString::AMFLongString()
{
}

AMFLongString::~AMFLongString()
{
}

/// @brief longstring类型由len + data. len4 字节
/// @param data 
/// @param size 
/// @param has 
/// @return 
int AMFLongString::Decode(const char *data, int size, bool has)
{
    // 够不够解析长度
    if(size < 4)
    {
        return -1;
    }

    auto len = BytesReader::ReadUint32T(data);
    if(len < 0 || size < len + 4)
    {
        return -1;
    }
    longString_.assign(data+4, len);
    return 4 + len;
}

bool AMFLongString::IsString()
{
    return true;  // 是LongString类，实现父类的判断函数，动态绑定就能执指向
}

const std::string &AMFLongString::String()
{
    return longString_;
}

void AMFLongString::Dump() const
{
    RTMP_TRACE << "LongString: " << longString_;
}
