/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 14:46:07
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 15:28:45
 * @FilePath: /tmms/src/base/StringUtils.h
 * @Description:  learn 
 */
#pragma once

#include <string>
#include <vector>

namespace tmms
{
    namespace base
    {
        using std::string;
        using std::vector;

        class StringUtils               // 工具类，所有函数成员定义成静态的
        {
        public:
            /// 1. 字符串匹配工具
            // 前缀匹配
            static bool StartsWith(const string& s, const string &sub);
            // 后缀匹配
            static bool EndWith(const string &s, const string &sub);
        
        public:
            /// 2. 文件名，文件路径操作
            // 从完整的文件路径，文件所在的父目录
            static string FilePath(const string& path);
            // 完整的文件路径中，取出文件名+文件后缀
            static string FileNameExt(const string &path);
            // 完整的文件路径中，返回文件名
            static string FileName(const string &path);
            // 完整的文文件路径中，返回文件后缀
            static string Extension(const string &path);

        public:
            /// 3. 字符串分割
            /// 把一个字符串按照分隔符分割成vector
            static vector<string> SplitString(const string &s, const string &delimiter);

            // 有限状态机
            static vector<string> SplitStringWithFSM(const string &s, const char delimiter);
        };
    }
}