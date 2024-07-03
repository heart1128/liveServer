/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-02 13:33:14
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-03 11:33:14
 * @FilePath: /liveServer/src/mmedia/http/HttpRequest.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include "HttpTypes.h"
#include <string>
#include <unordered_map>
#include <memory>

namespace tmms
{
    namespace mm
    {
        class HttpRequest
        {
        public:
        //  单参数的构造函数加，多个参数不会自动转换
            explicit HttpRequest(bool is_request = true);

        /// 消息头处理函数
            void AddHeader(const std::string &field, const std::string &value);
            void RemoveHeader(const std::string &key) ;
            const std::string &GetHeader(const std::string &field) const;
            void AddHeader(std::string &&field, std::string &&value);   
            const std::unordered_map<std::string, std::string>& Headers() const;
            std::string MakeHeaders();
        /// query函数
            void SetQuery(const std::string &query);
            void SetQuery(std::string &&query);
            void SetParameter(const std::string &key, const std::string &value);
            void SetParameter(std::string &&key, std::string &&value);
            const std::string &GetParameter(const std::string &key) const;
            const std::string &Query() const ;
        /// Method函数
            void SetMethod(const std::string &method);
            void SetMethod(HttpMethod method);
            HttpMethod Method() const;
        ///verion
            void SetVersion(Version v);
            void SetVersion(const std::string &version);
            Version GetVersion() const ;
            void SetPath(const std::string &path);
            const std::string &Path() const;
            void SetStatusCode(int32_t code);
            uint32_t GetStatusCode() const;
            void SetBody(const std::string &body);
            void SetBody(std::string &&body);
            const std::string &Body() const;
        /// 内容组装
            std::string AppendToBuffer();  // 请求组装成string
            bool IsRequest() const;
            bool IsStream() const;
            bool IsChunked() const;
            void SetIsStream(bool s);
            void SetIsChunked(bool c);

        
        private:
            void AppendRequestFirstLine(std::stringstream &ss);
            void AppendResponseFirstLine(std::stringstream &ss);
            void ParseParameters(); // 参数解析

            HttpMethod method_{kInvalid};
            Version version_{Version::kUnknown};
            std::string path_;
            std::string query_;
            std::unordered_map<std::string, std::string> headers_;
            std::unordered_map<std::string, std::string> parameters_;
            std::string body_;
            uint32_t code_{0};
            bool is_request_{true};
            bool is_stream_{false};  // 当前是流数据（没有读取到chunk header和没有body）
            bool is_chunk_{false};
        };
    } // namespace mm
    
} // namespace tmms
