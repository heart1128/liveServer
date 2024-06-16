/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 15:50:37
 */
#pragma once

#include "AMFAny.h"

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现Number数据类型的解析
        class AMFNumber : public AMFAny
        {
        public:
            AMFNumber(const std::string &name);
            AMFNumber();
            ~AMFNumber();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsNumber() override;
            double Number() override;
            void Dump() const override;

        private:
            double number_{0};
        };
    } // namespace mm
    
} // namespace tmms
