/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:18:01
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:30:50
 */
#include "AMFAny.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"

using namespace tmms::mm;

namespace
{
    std::string empty_string;
}

AMFAny::AMFAny(const std::string &name)
:name_(name)
{
}

AMFAny::AMFAny()
{
}

AMFAny::~AMFAny()
{
}

const std::string &AMFAny::String()
{
    if(this->IsString())
    {
        return this->String();
    }
    RTMP_ERROR << "not a String.";
    return empty_string;
}

bool AMFAny::Boolean()
{
    if(this->IsBoolean())
    {
        return this->Boolean();
    }
    RTMP_ERROR << "not a Boolean.";
    return false;
}

double AMFAny::Number()
{
    if(this->IsNumber())
    {
        return this->Number();
    }
    RTMP_ERROR << "not a Number.";
    return 0.0;
}

double AMFAny::Date()
{
    if(this->IsDate())
    {
        return this->Date();
    }
    RTMP_ERROR << "not a Date.";
    return 0.0;
}

AMFObjectPtr AMFAny::Object()
{
    if(this->IsObject())
    {
        return this->Object();
    }
    RTMP_ERROR << "not a Object.";
    return AMFObjectPtr();
}

bool AMFAny::IsString()
{
    
    return false;
}

bool AMFAny::IsNumber()
{
    return false;
}

bool AMFAny::IsBoolean()
{
    return false;
}

bool AMFAny::IsDate()
{
    return false;
}

bool AMFAny::IsObject()
{
    return false;
}

const std::string &AMFAny::Name() const
{
    return name_;
}

/// @brief 获取类型的属性个数
/// @return 基本类型都是1个属性，object有多个
int32_t AMFAny::Count() const
{
    return 1;
}

/// @brief 解析string类型的数据
/// @param data 
/// @return string数据
std::string AMFAny::DecodeString(const char *data)
{
    // string 类型 = 0x02 + UTF-8编码的字符串
    // UTF-8编码的字符串由16位的长度+字符串内容
    auto len = BytesReader::ReadUint16T(data);
    if(len > 0)
    {
        std::string str(data + 2, len);
        return str;
    }
    return std::string();
}
