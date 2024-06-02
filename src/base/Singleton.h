/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 15:32:38
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-01 17:51:09
 * @FilePath: /liveServer/src/base/Singleton.h
 * @Description:  learn 
 */
#pragma once
#include "NonCopyable.h"
#include <pthread.h>

namespace tmms
{
    namespace base
    {
        template <typename T>
        class Singleton : public NonCopyable        // 单例类
        {
        public:
            Singleton() = delete;
            ~Singleton() = delete;
        
        public:
            static T*& Instance();
        
        private:
            static void init();

            static pthread_once_t ponce_;           // pthread_once_t 保证变量在多个线程只初始化一次
            static T* value_;
        };

        template<typename T>
        pthread_once_t Singleton<T>::ponce_ = PTHREAD_ONCE_INIT;

        template<typename T>
        T *Singleton<T>::value_ = nullptr;

        template <typename T>
        T *&Singleton<T>::Instance()
        {
            // 如果ponce_还没有被初始化，就调用init函数
            // 如果已经被初始化过了，那就什么也不做，直接返回
            pthread_once(&ponce_, &Singleton::init);
            return value_;
        }

        template <typename T>
        void Singleton<T>::init()
        {
            if(!value_)
            {
                value_ = new T();
            }
        }

    } // namespace base

}// tmms
