/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-29 23:11:12
 */
#pragma once

#include "AMFAny.h"

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现Null数据类型的解析
        class AMFNull : public AMFAny
        {
        public:
            AMFNull(const std::string &name);
            AMFNull();
            ~AMFNull();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsNull() override;
            void Dump() const override;

        private:
            bool b_{false};
        };
    } // namespace mm
    
} // namespace tmms
