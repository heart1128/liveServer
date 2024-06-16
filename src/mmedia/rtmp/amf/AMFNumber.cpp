/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:29:04
 */
#include "AMFNumber.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

AMFNumber::AMFNumber(const std::string &name)
: AMFAny(name)
{
}

AMFNumber::AMFNumber()
{
}

AMFNumber::~AMFNumber()
{
}

/// @brief 解析NUmber类型的数据
/// @param data 
/// @param size 
/// @param has 
/// @return 返回读取成功的字节数，失败返回-1
int AMFNumber::Decode(const char *data, int size, bool has)
{
    if(size >= 8)
    {
        number_ = BytesReader::ReadUint64T(data);
        return 8;
    }
    return -1;
}

bool AMFNumber::IsNumber()
{
    return true;  // 是Number类，实现父类的判断函数，动态绑定就能执指向
}

double AMFNumber::Number()
{
    return number_;
}

void AMFNumber::Dump() const
{
    RTMP_TRACE << "Number: " << number_;
}
