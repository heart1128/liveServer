/*
 * @Description: 
 * @Version: 0.1
 * @Autor: 
 * @Date: 2024-06-16 15:42:19
 * @LastEditors: Please set LastEditors
 * @LastEditTime: 2024-06-16 17:38:25
 */
#pragma once

#include "AMFAny.h"
#include <vector>

namespace tmms
{
    namespace mm
    {
        /// @brief 继承AMFAny，实现Object数据类型的解析
        class AMFObject : public AMFAny
        {
        public:
            AMFObject(const std::string &name);
            AMFObject();
            ~AMFObject();
        
        public:
            int Decode(const char *data, int size, bool has = false) override;
            bool IsObject() override;
            AMFObjectPtr Object() override;
            void Dump() const override;

            int DecodeOnce(const char *data, int size, bool has = false);
            const AMFAnyPtr &Property(const std::string &name) const;
            const AMFAnyPtr &Property(int index) const;
        private:
            std::vector<AMFAnyPtr> properties_;
        };
    } // namespace mm
    
} // namespace tmms
