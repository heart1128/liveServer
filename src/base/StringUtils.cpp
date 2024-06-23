/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 14:46:21
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 15:51:49
 * @FilePath: /tmms/src/base/StringUtils.cpp
 * @Description:  learn 
 */
#include "StringUtils.h"

using namespace tmms::base;

/// @brief 子串头部匹配
/// @param s    源string
/// @param sub  子串
/// @return bool类型，是否存在头部子串匹配
bool StringUtils::StartsWith(const string &s, const string &sub)
{
    if(sub.empty())         // 子串为空，肯定存在
    {
        return true;
    }
    if(s.empty())           // 源串为空，肯定不匹配
    {
        return false;
    }
    auto len = s.size();
    auto slen = sub.size();
    if(len < slen)          // 子串更长，不匹配
    {
        return false;
    }

    return s.compare(0, slen, sub) == 0;    // compare是截取串长度和指定string比较，返回0是相等，不为0是不相等。
}

/// @brief 尾部子串匹配
/// @param s   源string
/// @param sub 子串
/// @return bool 类型，是否存在尾部子串匹配
bool StringUtils::EndWith(const string &s, const string &sub)
{
    if(sub.empty())         // 子串为空，肯定存在
    {
        return true;
    }
    if(s.empty())           // 源串为空，肯定不匹配
    {
        return false;
    }
    auto len = s.size();
    auto slen = sub.size();
    if(len < slen)          // 子串更长，不匹配
    {
        return false;
    }
    
    return s.compare(len - slen, slen, sub) == 0;    // compare是截取串长度和指定string比较，返回0是相等，不为0是不相等。
}

/// @brief 从完整的文件路径中，找到文件所在目录
/// @param path 
/// @return 文件所在目录的字符串
string StringUtils::FilePath(const string &path)
{
    auto pos = path.find_last_of("/\\");        // 找最后一个目录的位置 /
    if(pos != string::npos)                
    {
        return path.substr(0, pos);             // 返回前面的目录路径
    }
    else
    {
        return "./";
    }

    return "";
}

/// @brief 从完整的路径中返回文件名+后缀
/// @param path 
/// @return 文件名+后缀
string StringUtils::FileNameExt(const string &path)
{
    auto pos = path.find_last_of("/\\");
    if(pos != string::npos)
    {
        if(pos + 1 < path.size())               // 是不是找到了最后一个字符
        {
            return path.substr(pos + 1);
        }
    }
    
    return path;                                // 没有目录，就是全部路径
}

/// @brief 从完整的目录路径，找到文件名
/// @param path 
/// @return 返回文件名，不带后缀
string StringUtils::FileName(const string &path)
{
    string file_name = FileNameExt(path);
    auto pos = file_name.find_last_of(".");
    if(pos != string::npos)
    {
        if(pos != 0)                            // 文件名第一个字符不是.，说明存在后缀
        {
            return file_name.substr(0, pos);
        }
    }

    return file_name;                           // 不存在后缀，返回全部文件名
}

/// @brief 完整的路径中，找到文件的后缀
/// @param path 
/// @return 返回文件后缀的字符串
string StringUtils::Extension(const string &path)
{
    string file_name = FileNameExt(path);
    auto pos = file_name.find_last_of(".");
    if(pos != string::npos)
    {
        if(pos != 0 && pos + 1 < file_name.size()) // 文件名第一个字符不是.或者.不是最后一个，说明存在后缀
        {
            return file_name.substr(pos + 1);
        }
    }

    return file_name;                           // 不存在后缀，返回全部文件名
}

/// @brief 把一个字符串按照分隔符分割成子串
/// @param s            源串
/// @param delimiter    分隔符
/// @return 返回一个vector包裹的全部子串
vector<string> StringUtils::SplitString(const string &s, const string &delimiter)
{
    if(delimiter.empty())           // 分隔符为空，返回空的列表
    {
        return vector<string>{};
    }

    vector<string> result;          // 这个临时变量如果是返回的，编译器会进行优化，空间申请在返回值内，不会有额外的复制开销
    size_t last = 0;
    size_t next = 0;
    while((next = s.find(delimiter, last)) != string::npos)  // 不断查找下一个分隔符
    {
        if(next > last)
        {
            result.emplace_back(s.substr(last, next - last));   // 子串分割
        }
        else
        {
            result.emplace_back("");    // 比如//之间的空字符串也要计算
        }
        last = next + delimiter.size();
    }

    // 处理分隔符到末尾的子串
    if(last < s.size())
    {
        result.emplace_back(s.substr(last));        // 不指定长度，默认到最后
    }

    return result;
}

vector<string> StringUtils::SplitStringWithFSM(const string &s, const char delimiter)
{
    enum
    {
        kStateInit = 0,
        kStateNormal = 1,
        kStateDelimiter = 2,
        kStateEnd = 3,
    };

    int state = kStateInit;
    vector<string> result;          // 这个临时变量如果是返回的，编译器会进行优化，空间申请在返回值内，不会有额外的复制开销
    std::string tmp;

    state = kStateNormal;
    for(int pos = 0; pos < s.size();)
    {
        if(state == kStateNormal)
        {
            if(s.at(pos) == delimiter)      // 分隔符状态转移，跳过字符
            {
                state = kStateDelimiter;
                continue;
            }
            tmp.push_back(s.at(pos));   // 非分隔符，保存字符
            pos++;
        }
        else if(state == kStateDelimiter)       // 上一个状态是分隔符状态，这里就要保存上一段字符，然后重新找分隔符
        {
            result.push_back(tmp);  // 空的tmp也加进去，
            tmp.clear();
            state = kStateNormal;
            pos++;
        }
    }
    if(!tmp.empty())    // 可能最后是一个分隔符
    {
        result.push_back(tmp);
    }
    state = kStateEnd;
    return result;
}
