/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:29:17
 */
#include "AMFString.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

AMFString::AMFString(const std::string &name)
: AMFAny(name)
{
}

AMFString::AMFString()
{
}

AMFString::~AMFString()
{
}

/// @brief string类型由len + data. len2字节
/// @param data 
/// @param size 
/// @param has  // 有没有属性名
/// @return 
int AMFString::Decode(const char *data, int size, bool has)
{
    // 够不够解析长度
    if(size < 2)
    {
        return -1;
    }

    auto len = BytesReader::ReadUint16T(data);
    if(len < 0 || size < len + 2)
    {
        return -1;
    }
    string_ = DecodeString(data);
    return 2 + string_.size();
}

bool AMFString::IsString()
{
    return true;  // 是String类，实现父类的判断函数，动态绑定就能执指向
}

const std::string &AMFString::String()
{
    return string_;
}

void AMFString::Dump() const
{
    RTMP_TRACE << "String: " << string_;
}
