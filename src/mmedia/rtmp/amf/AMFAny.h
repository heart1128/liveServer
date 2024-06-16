#pragma once

#include <memory>
#include <string>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        enum AMFDataType // 不会全部用到。，AMF携带的数据类型定义
        { 
            kAMFNumber = 0, 
            kAMFBoolean, 
            kAMFString, 
            kAMFObject,
            kAMFMovieClip,		/* reserved, not used */
            kAMFNull, 
            kAMFUndefined, 
            kAMFReference, 
            kAMFEcmaArray, 
            kAMFObjectEnd,
            kAMStrictArray, 
            kAMFDate, 
            kAMFLongString, 
            kAMFUnsupported,
            kAMFRecordset,		/* reserved, not used */
            kAMFXMLDoc, 
            kAMFTypedObject,
            kAMFAvmplus,		/* switch to AMF3 */
            kAMFInvalid = 0xff,
        };

        class AMFObject;
        class AMFAny;
        using AMFObjectPtr = std::shared_ptr<AMFObject>;
        using AMFAnyPtr = std::shared_ptr<AMFAny>;

        class AMFAny : public std::enable_shared_from_this<AMFAny>
        {
        public:
            AMFAny(const std::string &name);
            AMFAny();
            virtual ~AMFAny();

        public:
            /// @brief 解码
            /// @param data  需要解析的数据缓存地址
            /// @param size  需要解析的数据缓存大小
            /// @param has   是否带名字
            /// @return >=0, 解析成功的字节数；-1，解析出错
            virtual int Decode(const char *data, int size, bool has = false) = 0;

            // 由子类具体实现，返回对应类型的数据
            virtual const std::string &String();
            virtual bool Boolean();
            virtual double Number();
            virtual double Date();
            virtual AMFObjectPtr Object();

            // 不是每个类都要重新实现的这个函数的，作为一个通用函数
            virtual bool IsString();
            virtual bool IsNumber();
            virtual bool IsBoolean();
            virtual bool IsDate();
            virtual bool IsObject();

            virtual void Dump() const = 0;
            const std::string &Name() const;
            virtual int32_t Count() const;

        protected:
            static std::string DecodeString(const char *data);
        private:
            std::string name_;
        };
    } // namespace mm
       
} // namespace tmms
