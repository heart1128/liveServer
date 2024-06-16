/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:28:48
 */
#include "AMFBoolean.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

AMFBoolean::AMFBoolean(const std::string &name)
:AMFAny(name)
{
}

AMFBoolean::AMFBoolean()
{
}

AMFBoolean::~AMFBoolean()
{
}

int AMFBoolean::Decode(const char *data, int size, bool has)
{
    if(size >= 1)
    {
        b_ = *data != 0 ? true : false;
        return 1;
    }
    return -1;
}

bool AMFBoolean::IsBoolean()
{
    return true;  // 是Boolean类，实现父类的判断函数，动态绑定就能执指向
}

bool AMFBoolean::Boolean()
{
    return b_;
}

void AMFBoolean::Dump() const
{
    RTMP_TRACE << "Boolean: " << b_;
}
