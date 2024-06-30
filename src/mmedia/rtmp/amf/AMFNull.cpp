/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 23:13:47
 */
#include "AMFNull.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

AMFNull::AMFNull(const std::string &name)
:AMFAny(name)
{
}

AMFNull::AMFNull()
{
}

AMFNull::~AMFNull()
{
}

/// @brief  NULL不用解析，直接返回0表示解析正确
/// @param data 
/// @param size 
/// @param has 
/// @return 
int AMFNull::Decode(const char *data, int size, bool has)
{
    return 0;
}

bool AMFNull::IsNull()
{
    return true;  // 是Null类，实现父类的判断函数，动态绑定就能执指向
}

void AMFNull::Dump() const
{
    RTMP_TRACE << "Null: ";
}
