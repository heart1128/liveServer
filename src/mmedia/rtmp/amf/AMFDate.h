/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:38:08
 */
#pragma once

#include "AMFAny.h"

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现Date数据类型的解析
        class AMFDate : public AMFAny
        {
        public:
            AMFDate(const std::string &name);
            AMFDate();
            ~AMFDate();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsDate() override;
            double Date() override;
            void Dump() const override;

            int16_t UtcOffset() const;

        private:
            double utc_{0.0f};
            int16_t utc_offset_{0};
        };
    } // namespace mm
    
} // namespace tmms
