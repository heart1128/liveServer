/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:38:29
 */
#pragma once

#include "AMFAny.h"

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现String数据类型的解析
        class AMFString : public AMFAny
        {
        public:
            AMFString(const std::string &name);
            AMFString();
            ~AMFString();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsString() override;
            const std::string &String() override;
            void Dump() const override;

        private:
            std::string string_;
        };
    } // namespace mm
    
} // namespace tmms
