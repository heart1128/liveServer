/*
 * @Description:  
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:46:07
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 23:14:31
 */
#include "AMFObject.h"
#include "mmedia/base/MMediaLog.h"
#include "mmedia/base/BytesReader.h"
#include "AMFNumber.h"
#include "AMFString.h"
#include "AMFBoolean.h"
#include "AMFDate.h"
#include "AMFNull.h"
#include "AMFLongString.h"

using namespace tmms::mm;

namespace
{
    static AMFAnyPtr any_ptr_null;
}

AMFObject::AMFObject(const std::string &name)
: AMFAny(name)
{
}

AMFObject::AMFObject()
{
}

AMFObject::~AMFObject()
{
}

/// @brief 一个object是键值对，可以解析多个属性
/// @param data 
/// @param size 
/// @param has  有没有属性名，就是是不是object对象 
/// @return 
int AMFObject::Decode(const char *data, int size, bool has)
{
    std::string name;
    int32_t parsed = 0; // 已经解析的字节数
    
    // 一个data里面就是所有属性的值，根据属性的类型解析相应的结构值
    while((parsed + 3) <= size)
    {
        if(BytesReader::ReadUint24T(data) == 0x000009)
        {
            parsed += 3;
            return parsed;
        }
        if(has) // 如果有属性名
        {
            // name是String类型
            name = DecodeString(data);
            if(!name.empty())
            {
                parsed += (name.size() + 2);
                data += (name.size() + 2);
            }
        }
        char type = *data++;
        parsed++;
        switch (type)
        {
            case kAMFNumber:
            {
                std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(name);
                auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "Number value:" << p->Number();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFBoolean:
            {
                std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(name);
                auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "Boolean value:" << p->Boolean();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFString:
            {
                std::shared_ptr<AMFString> p = std::make_shared<AMFString>(name);
                auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "String value:" << p->String();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFObject:    // 就是递归object属性
            {
                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
                auto len = p->Decode(data, size - parsed, true); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "Object :";
                p->Dump();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFNull:
            {   
                // fixbug：之前这里都没有把NULL写入Object的属性，所以按索引取属性的时候少了一个，取错了
                RTMP_TRACE << "Null.";
                properties_.emplace_back(std::move(std::make_shared<AMFNull>()));
                break;
            }
            case kAMFEcmaArray: // 对象数组
            {
                int count = BytesReader::ReadUint32T(data);
                parsed += 4;
                data += 4;

                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
                auto len = p->Decode(data, size - parsed, true); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "EcmaArray :";
                p->Dump();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFObjectEnd: // 处理到object的结尾了
            {
                return parsed; 
            }
            case kAMStrictArray:
            {
                int count = BytesReader::ReadUint32T(data);
                parsed += 4;
                data += 4;

                while(count--)
                {
                    std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
                    auto len = p->DecodeOnce(data, size - parsed, true); // 剩下的部分都是数据
                    if(len == -1)
                    {
                        return -1;
                    }
                    data += len;
                    parsed += len;
                    RTMP_TRACE << "StrictArray :";
                    p->Dump();
                    properties_.emplace_back(std::move(p));
                }
                break;
            }
            case kAMFDate:
            {
                std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(name);
                auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "Date value:" << p->Date();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFLongString:
            {
                std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(name);
                auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "LongString value:" << p->String();
                properties_.emplace_back(std::move(p));
                break;
            }
            case kAMFMovieClip:
            case kAMFUndefined:
            case kAMFReference:
            case kAMFUnsupported:
            case kAMFRecordset:
            case kAMFXMLDoc:
            case kAMFTypedObject:
            case kAMFAvmplus:
            {
                RTMP_TRACE << " not surpport type:" << type;
                break;
            }
            default:
                break;
        }
    }
    return parsed;
}

bool AMFObject::IsObject()
{
    return true;  // 是Object类，实现父类的判断函数，动态绑定就能执指向
}

AMFObjectPtr AMFObject::Object()
{
    return std::dynamic_pointer_cast<AMFObject>(shared_from_this());
}

void AMFObject::Dump() const
{
    RTMP_TRACE << "Object start";
    for(auto const &p : properties_)
    {
        p->Dump();
    }
    RTMP_TRACE << "Object end";
}

// 和Decode的区别就是只解析一次
int AMFObject::DecodeOnce(const char *data, int size, bool has)
{
    std::string name;
    int32_t parsed = 0; // 已经解析的字节数
    
    // 一个data里面就是所有属性的值，根据属性的类型解析相应的结构值
    if(has) // 如果有属性名
    {
        // name是String类型
        name = DecodeString(data);
        if(!name.empty())
        {
            parsed += (name.size() + 2);
            data += (name.size() + 2);
        }
    }
    char type = *data++;
    parsed++;
    switch (type)
    {
        case kAMFNumber:
        {
            std::shared_ptr<AMFNumber> p = std::make_shared<AMFNumber>(name);
            auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "Number value:" << p->Number();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFBoolean:
        {
            std::shared_ptr<AMFBoolean> p = std::make_shared<AMFBoolean>(name);
            auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "Boolean value:" << p->Boolean();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFString:
        {
            std::shared_ptr<AMFString> p = std::make_shared<AMFString>(name);
            auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "String value:" << p->String();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFObject:    // 就是递归object属性
        {
            std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
            auto len = p->Decode(data, size - parsed, true); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "Object :";
            p->Dump();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFNull:
        {
            RTMP_TRACE << "Null.";
            break;
        }
        case kAMFEcmaArray: // 对象数组
        {
            int count = BytesReader::ReadUint32T(data);
            parsed += 4;
            data += 4;

            std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
            auto len = p->Decode(data, size - parsed, true); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "EcmaArray :";
            p->Dump();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFObjectEnd: // 处理到object的结尾了
        {
            return parsed; 
        }
        case kAMStrictArray:
        {
            int count = BytesReader::ReadUint32T(data);
            parsed += 4;
            data += 4;

            while(count--)
            {
                std::shared_ptr<AMFObject> p = std::make_shared<AMFObject>(name);
                auto len = p->DecodeOnce(data, size - parsed, true); // 剩下的部分都是数据
                if(len == -1)
                {
                    return -1;
                }
                data += len;
                parsed += len;
                RTMP_TRACE << "StrictArray :";
                p->Dump();
                properties_.emplace_back(std::move(p));
            }
            break;
        }
        case kAMFDate:
        {
            std::shared_ptr<AMFDate> p = std::make_shared<AMFDate>(name);
            auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "Date value:" << p->Date();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFLongString:
        {
            std::shared_ptr<AMFLongString> p = std::make_shared<AMFLongString>(name);
            auto len = p->Decode(data, size - parsed); // 剩下的部分都是数据
            if(len == -1)
            {
                return -1;
            }
            data += len;
            parsed += len;
            RTMP_TRACE << "LongString value:" << p->String();
            properties_.emplace_back(std::move(p));
            break;
        }
        case kAMFMovieClip:
        case kAMFUndefined:
        case kAMFReference:
        case kAMFUnsupported:
        case kAMFRecordset:
        case kAMFXMLDoc:
        case kAMFTypedObject:
        case kAMFAvmplus:
        {
            RTMP_TRACE << " not surpport type:" << type;
            break;
        }
        default:
            break;
    }
    return parsed;
}

/// @brief 按属性名访问
/// @param name 
/// @return 
const AMFAnyPtr &AMFObject::Property(const std::string &name) const
{
    for(const auto& p : properties_)
    {
        if(p->Name() == name)       // 简单属性才有 name，对象没有
        {
            return p;
        }
        else if(p->IsObject())  // 是对象
        {
            AMFObjectPtr obj = p->Object();
            const AMFAnyPtr &p2 = obj->Property(name); // 是对象就递归查找
            if(p2)
            {
                return p2;
            }
        }
    }

    return any_ptr_null;
}

/// @brief 按索引访问
/// @param index 
/// @return 
const AMFAnyPtr &AMFObject::Property(int index) const
{
    if(index < 0 || index >= properties_.size())
    {
        return any_ptr_null;
    }

    return properties_[index];
}
