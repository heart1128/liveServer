/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-02 13:37:15
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-03 16:59:10
 * @FilePath: /liveServer/src/mmedia/http/HttpRequest.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#include "HttpRequest.h"
#include "mmedia/base/MMediaLog.h"
#include "base/StringUtils.h"
#include "HttpUtils.h"
#include <algorithm>

using namespace tmms::mm;
namespace
{
    static std::string string_empty;
}

HttpRequest::HttpRequest(bool is_request)
:is_request_(is_request)
{
}

void HttpRequest::AddHeader(const std::string &field, const std::string &value)
{
    std::string k = field;
    // 全部转换为小写,从头开始
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    headers_[k] = value;
}

void HttpRequest::RemoveHeader(const std::string &key)
{
    std::string k = key;
    // 全部转换为小写,从头开始
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);
    headers_.erase(key);

}

const std::string &HttpRequest::GetHeader(const std::string &field) const
{
    std::string k = field;
    // 全部转换为小写,从头开始
    std::transform(k.begin(), k.end(), k.begin(), ::tolower);

    auto iter = headers_.find(k);
    if(iter != headers_.end())
    {
        return iter->second;
    }
    return string_empty;
}

void HttpRequest::AddHeader(std::string &&field, std::string &&value)
{
    std::transform(field.begin(), field.end(), field.begin(), ::tolower);
    headers_[std::move(field)] = std::move(value);
}

const std::unordered_map<std::string, std::string> &HttpRequest::Headers() const
{
    return headers_;
}

/// @brief 制作头
/// @return 
std::string HttpRequest::MakeHeaders()
{
    std::stringstream ss;
    // 请求行
    if(is_request_)
    {
        AppendRequestFirstLine(ss);
    }
    else
    {
        AppendResponseFirstLine(ss);
    }
    // 请求头
    for(auto const &h : headers_)
    {
        ss << h.first << ":" << h.second << "\r\n";
    }

    // 如果是stream不不能响应长度
    // if(!body_.empty())
    // {
    //     ss << "content-length:" << body_.size() << "\r\n";
    // }
    // else
    // {
    //     ss << "content-length: 0\r\n";
    // }
    ss << "\r\n";
    return ss.str();
}

void HttpRequest::SetQuery(const std::string &query)
{
    query_ = query;
    ParseParameters();// 需要解析一下，可能参数还是没有编码的
}

void HttpRequest::SetQuery(std::string &&query)
{
    query_ = std::move(query);
    ParseParameters();
}

/// @brief param是自定义的查询字符串，不能转换大小写
/// @param key 
/// @param value 
void HttpRequest::SetParameter(const std::string &key, const std::string &value)
{
    parameters_[key] = value;
}

void HttpRequest::SetParameter(std::string &&key, std::string &&value)
{
    parameters_[std::move(key)] = std::move(value);
}

const std::string &HttpRequest::GetParameter(const std::string &key) const
{
    auto iter = parameters_.find(key);
    if(iter != parameters_.end())
    {
        return iter->second;
    }
    return string_empty;
}

const std::string &HttpRequest::Query() const
{
    return query_;
}

void HttpRequest::SetMethod(const std::string &method)
{
    method_ = HttpUtils::ParseMethod(method);
}

void HttpRequest::SetMethod(HttpMethod method)
{
    method_ = method;
}

HttpMethod HttpRequest::Method() const
{
    return method_;
}

void HttpRequest::SetVersion(Version v)
{
    version_ = v;
}

void HttpRequest::SetVersion(const std::string &v)
{
    version_ = Version::kUnknown;
    if(v.size() == 8) // http/1.0
    {
        if(v.compare(0, 6, "HTTP/1."))  // http 1
        {
            if(v[7] == '1') // http 1.1
            {
                version_ = Version::kHttp11;
            }
            else if(v[7] == '0')
            {
                version_ = Version::kHttp10;
            }
        }
    }
}

Version HttpRequest::GetVersion() const
{
    return version_;
}

void HttpRequest::SetPath(const std::string &path)
{
    // 如果是使用编码的path数据，就需要解码
    if(HttpUtils::NeedUrlDecoding(path))
    {
        path_ = HttpUtils::UrlDecode(path);
    }
    else
    {
        path_ = path;
    }
}

const std::string &HttpRequest::Path() const
{
    return path_;
}

void HttpRequest::SetStatusCode(int32_t code)
{
    code_ = code;
}

uint32_t HttpRequest::GetStatusCode() const
{
    return code_;
}

void HttpRequest::SetBody(const std::string &body)
{
    body_ = body;
}

void HttpRequest::SetBody(std::string &&body)
{
    body_ = std::move(body);
}

const std::string &HttpRequest::Body() const
{
    return body_;
}

/// @brief  组装整个http请求
/// @return 
std::string HttpRequest::AppendToBuffer()
{
    std::stringstream ss;
    ss << MakeHeaders();
    if(!body_.empty())
    {
        ss << body_;
    }
    return ss.str();
}

bool HttpRequest::IsRequest() const
{
    return is_request_;
}

bool HttpRequest::IsStream() const
{
    return is_stream_;
}

bool HttpRequest::IsChunked() const
{
    return is_chunk_;
}

void HttpRequest::SetIsStream(bool s)
{
    is_stream_ = s;
}

void HttpRequest::SetIsChunked(bool c)
{
    is_chunk_ = c;
}

/// @brief 组装请求行 method url version \r\n
/// @param ss 写入到这个流
void HttpRequest::AppendRequestFirstLine(std::stringstream &ss)
{
    switch (method_)
    {
        case kGet:
        {
            // 后面空格必须带
            ss << "GET ";
            break;
        }
        case kPost:
        {
            ss << "POST ";
            break;
        }
        case kHead:
        {
            ss << "HEAD ";
            break;
        }
        case kPut:
        {
            ss << "PUT ";
            break;
        }
        case kDelete:
        {
            ss << "DELETE ";
            break;
        }
        case kOptions:
        {
            ss << "OPTIONS ";
            break;
        }
        case kPatch:
        {
            ss << "PATCH ";
            break;
        }
        default:
        {
            ss << "UNKONW ";
            break;
        }
    }

    // url   path?k=v&k1=v1
    std::stringstream sss;
    if(!path_.empty())
    {
        sss << path_;
    }
    else
    {
        sss << "/";
    }
    if(!parameters_.empty())
    {
        sss << "?";
        for(auto iter = parameters_.begin(); iter != parameters_.end(); ++iter)
        {
            if(iter == parameters_.begin())
            {
                sss << iter->first << "=" << iter->second;
            }
            else
            {
                sss << "&" << iter->first << "=" << iter->second;
            }
            
        }
    }
        // url进行编码
    ss << HttpUtils::UrlEncode(sss.str()) << " ";

    // version
    if(version_ == Version::kHttp10)
    {
        ss << "HTTP/1.0 ";
    }
    else
    {
        ss << "HTTP/1.1 ";
    }
    ss << "\r\n";
}

/// @brief 组装响应行 version code codedesc \r\n
/// @param ss 
void HttpRequest::AppendResponseFirstLine(std::stringstream &ss)
{
    // version
    if(version_ == Version::kHttp10)
    {
        ss << "HTTP/1.0 ";
    }
    else
    {
        ss << "HTTP/1.1 ";
    }
    // code
    ss << code_ << " ";

    // code desc
    ss << HttpUtils::ParseStatusCode(code_);
    ss << "\r\n";
}

void HttpRequest::ParseParameters()
{
    auto list = base::StringUtils::SplitString(query_, "&");
    for(auto const & l : list)
    {
        //参数&key=value
        auto pos = l.find("=");
        if(pos != std::string::npos)
        {
            std::string k = l.substr(0, pos);
            std::string v = l.substr(pos + 1);
            k = HttpUtils::Trim(k); // 去前后空格
            v = HttpUtils::Trim(v);
            SetParameter(std::move(k), std::move(v));
        }
    }
}
