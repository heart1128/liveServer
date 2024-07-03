/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-02 13:30:16
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-02 13:30:20
 * @FilePath: /liveServer/src/mmedia/http/HttpUtils.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once
#include "HttpTypes.h"
#include <cstdint>
#include <vector>
#include <sstream>
#include <functional>
#include <string>
#include <unordered_map>
#include <algorithm>

namespace tmms
{
    namespace mm
    {
        class HttpUtils
        {
        public:
            static HttpMethod ParseMethod(const std::string &method);
            static HttpStatusCode ParseStatusCode(int32_t code);
            static std::string ParseStatusMessage(int32_t code);
            static ContentType ParseContentType(const std::string &contentType);
            static const std::string &ContentTypeToString(ContentType contenttype);
            static const std::string &StatusCodeToString(int code);
            static ContentType GetContentType(const std::string &fileName);
            static std::string CharToHex(char c);
            static bool NeedUrlDecoding(const std::string &url);
            static std::string UrlDecode(const std::string &url);      
            static std::string UrlEncode(const std::string &src);

            static std::string& ltrim(std::string &str) 
            {
                auto p = std::find_if(str.begin(), str.end(), std::not1(std::ptr_fun<int, int>(std::isspace)));
                str.erase(str.begin(), p);
                return str;
            }

            static std::string& rtrim(std::string &str) 
            {
                auto p = std::find_if(str.rbegin(), str.rend(), std::not1(std::ptr_fun<int , int>(std::isspace)));
                str.erase(p.base(), str.end());
                return str;
            }
            
            static std::string& Trim(std::string &str) 
            {
                ltrim(rtrim(str));
                return str;
            }     
        };          
    }
}