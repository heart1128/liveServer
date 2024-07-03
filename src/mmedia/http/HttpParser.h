/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-01 11:12:12
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-02 15:09:05
 * @FilePath: /liveServer/src/mmedia/http/HttpParser.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */

#pragma once

#include "mmedia/base/Packet.h"
#include "HttpTypes.h"
#include "network/base/MsgBuffer.h"
#include "HttpRequest.h"
#include <unordered_map>
#include <string>
#include <memory>

namespace tmms
{
    namespace mm
    {
        using namespace tmms::network;
        // http状态机，三类数据 
        // 1. http body
        // 2. chunk  设置chunk size
        // 3. stream 没有边界，收到指定大小为一个包
        enum HttpParserState 
        {
            kExpectHeaders,
            kExpectNormalBody,
            kExpectStreamBody,
            kExpectHttpComplete,
            kExpectChunkLen,
            kExpectChunkBody,
            kExpectChunkComplete,
            kExpectLastEmptyChunk,

            kExpectContinue,
            kExpectError,
        };

        using HttpRequestPtr = std::shared_ptr<HttpRequest>;

        class HttpParser
        {
        public:
            HttpParser() = default;
            ~HttpParser() = default;

        public:
            HttpParserState Parse(MsgBuffer &buf); // 解析
            const PacketPtr &Chunk() const ;        // 返回chunk
            HttpStatusCode Reason() const;
            void ClearForNextHttp(); // 一个请求完成之后清除所有状态
            void ClearForNextChunk();   // 收到chunk之后要清理状态

            HttpRequestPtr GetHttpRequest() const
            {
                return req_;
            }

        private:
            void ParseStream(MsgBuffer &buf); 
            void ParseNormalBody(MsgBuffer &buf);   // http普通的body
            void ParseChunk(MsgBuffer &buf);
            void ParseHeaders();
            void ProcessMethodLine(const std::string &line);    // 解析状态行


            HttpParserState state_{kExpectHeaders};
            int32_t current_chunk_length_{0};
            int32_t current_content_length_{0};
            bool is_stream_{false};  // 当前是流数据（没有读取到chunk header和没有body）
            bool is_chunk_{false};
            bool is_request_{true}; //  请求还是相应
            HttpStatusCode reason_{kUnknown};    // 状态码,解析失败的原因

            HttpRequestPtr req_;
            std::string header_;        // 当前头
            // std::unordered_map<std::string, std::string> headers_; // 保存所有数据包的头
            // std::string method_; // 方法 ,get, post...
            // uint32_t code_{kUnknown}; // 相应的状态码
            // std::string version_; // http版本
            // std::string path_; // 请求路径
            // std::string query_; // 查询参数
            PacketPtr chunk_;   // 包数据
        };
        
    } // namespace mm
    
} // namespace tmms
