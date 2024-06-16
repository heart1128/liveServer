/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:37:54
 */
#pragma once

#include "AMFAny.h"

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现Boolean数据类型的解析
        class AMFBoolean : public AMFAny
        {
        public:
            AMFBoolean(const std::string &name);
            AMFBoolean();
            ~AMFBoolean();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsBoolean() override;
            bool Boolean() override;
            void Dump() const override;

        private:
            bool b_{false};
        };
    } // namespace mm
    
} // namespace tmms
