/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 15:28:24
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 15:31:37
 * @FilePath: /liveServer/src/base/NonCopyable.h
 * @Description:  learn
 */
#pragma once

namespace tmms
{
    namespace base
    {
        class NonCopyable
        {
        protected:
            NonCopyable()
            {}
            ~NonCopyable()
            {}

            // c++  3/5法则，有默认的拷贝构造，拷贝赋值运算符，析构，移动构造，移动赋值
            // 定义了其中一个，默认生成其他四个。删除其中一个，默认不会生成其他四个
            NonCopyable(const NonCopyable&) = delete;
            NonCopyable &operator=(const NonCopyable&) = delete;
        };   
    } // namespace base
} // namespace tmms
