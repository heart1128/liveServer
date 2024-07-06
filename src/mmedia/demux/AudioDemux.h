/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-05 13:49:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-05 13:50:33
 * @FilePath: /liveServer/src/mmedia/demux/AudioDemux.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <list>
#include <cstdint>

namespace tmms
{
    namespace mm
    {
        class AudioDemux
        {
        public:
            AudioDemux() = default;
            ~AudioDemux() = default;
        
            int32_t OnDemux(const char *data, size_t size, std::list<SampleBuf> &list);

        private:
        };
    } // namespace mm
    
} // namespace tmms
