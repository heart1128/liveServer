/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-16 16:01:32
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-16 17:35:54
 * @FilePath: /liveServer/src/mmedia/hls/FragmentWindow.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#pragma once

#include "Fragment.h"
#include <cstdint>
#include <string>
#include <mutex>
#include <vector>
#include <memory>

namespace tmms
{
    namespace mm
    {

        using FragmentPtr = std::shared_ptr<Fragment>;

        /// @brief 管理切片的窗口，窗口内是有效的多个切片，窗口外的就是无效的，因为不能一直保存在内存中
        class FragmentWindow
        {
        public:
            FragmentWindow(int32_t size = 5);
            ~FragmentWindow();

            /**
             * @description: 添加新片段到窗口中，内部会自动移动窗口，更新m3u8文件内容
             * @param {FragmentPtr} &  片段
             * @return {*}
            **/            
            void AppendFragment(FragmentPtr &&fragment);
            FragmentPtr GetIdleFragment();
            const FragmentPtr &GetFragmentByName(const std::string &name);
            std::string GetPlayList();  // m3u8
        
        private:
            void Shrink();  // 收缩window
            void UpdatePlayList();  // 就是写m3u8的文件属性
        
        private:
            int32_t window_size_{5}; // 5个切片
            std::vector<FragmentPtr> fragments_;
            std::vector<FragmentPtr> free_fragments_;
            std::string playlist_;  // m3u8文件内容
            std::mutex lock_;
        };
        
    } // namespace mm
    
} // namespace tmms
