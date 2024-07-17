/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-10 10:43:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-17 13:50:02
 * @FilePath: /liveServer/src/mmedia/mpegts/StreamWriter.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include <cstdint>
#include <string>

namespace tmms
{
    namespace mm
    {
        const int32_t kSectionMaxSize = 1020;
        
        // 后续实现不同写
        class StreamWriter
        {
        public:
            StreamWriter(){}
            virtual ~StreamWriter(){}
            
            virtual void AppendTimeStamp(int64_t dts) = 0;
            virtual int32_t Write(void* buf, uint32_t size) = 0;
            virtual char* Data() = 0;
            virtual int Size() = 0;

            /**
             * @description:   最终这个类是被hls切片继承，也就是每个切片需要发送一次sps和pps，不是每个idr都要发送
             * @param {string} &sps
             * @return {*}
            **/            
            void SetSPS(const std::string &sps)
            {
                sps_ = sps;
            }
            void SetPPS(const std::string &pps)
            {
                pps_ = pps;
            }
            const std::string &GetSPS() const
            {
                return sps_;
            }
            const std::string &GetPPS() const
            {
                return pps_;
            }
            void SetSpsPpsAppended(bool b)
            {
                sps_pps_appended_ = b;
            }

            bool GetSpsPpsAppended() const
            {
                return sps_pps_appended_;
            }
        
        protected:
            std::string sps_;
            std::string pps_;
            bool sps_pps_appended_{false};
        }; 

    }
}