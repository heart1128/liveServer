/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-01 11:19:58
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-01 11:20:22
 * @FilePath: /liveServer/src/mmedia/http/HttpTypes.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

namespace tmms
{
    namespace mm
    {
        enum HttpStatusCode
        {
            kUnknown = 0,
            k100Continue = 100,
            k101SwitchingProtocols = 101,
            k200OK = 200,
            k201Created = 201,
            k202Accepted = 202,
            k203NonAuthoritativeInformation = 203,
            k204NoContent = 204,
            k205ResetContent = 205,
            k206PartialContent = 206,
            k300MultipleChoices = 300,
            k301MovedPermanently = 301,
            k302Found = 302,
            k303SeeOther = 303,
            k304NotModified = 304,
            k305UseProxy = 305,
            k307TemporaryRedirect = 307,
            k308PermanentRedirect = 308,
            k400BadRequest = 400,
            k401Unauthorized = 401,
            k402PaymentRequired = 402,
            k403Forbidden = 403,
            k404NotFound = 404,
            k405MethodNotAllowed = 405,
            k406NotAcceptable = 406,
            k407ProxyAuthenticationRequired = 407,
            k408RequestTimeout = 408,
            k409Conflict = 409,
            k410Gone = 410,
            k411LengthRequired = 411,
            k412PreconditionFailed = 412,
            k413RequestEntityTooLarge = 413,
            k414RequestURITooLarge = 414,
            k415UnsupportedMediaType = 415,
            k416RequestedRangeNotSatisfiable = 416,
            k417ExpectationFailed = 417,
            k418ImATeapot = 418,
            k421MisdirectedRequest = 421,
            k425TooEarly = 425,
            k426UpgradeRequired = 426,
            k428PreconditionRequired = 428,
            k429TooManyRequests = 429,
            k431RequestHeaderFieldsTooLarge = 431,
            k451UnavailableForLegalReasons = 451,
            k500InternalServerError = 500,
            k501NotImplemented = 501,
            k502BadGateway = 502,
            k503ServiceUnavailable = 503,
            k504GatewayTimeout = 504,
            k505HTTPVersionNotSupported = 505,
            k510NotExtended = 510,
        };

        enum class Version
        {
            kUnknown = 0,
            kHttp10,
            kHttp11
        };

        enum ContentType
        {
            kContentTypeNONE = 0,
            kContentTypeAppJson,
            kContentTypeTextPlain,
            kContentTypeTextHTML,
            kContentTypeAppXForm,
            kContentTypeAppMpegUrl,
            kContentTypeVideoMP2T,
            kContentTypeVideoXFlv,
            kContentTypeTextXML,
            kContentTypeAppXML,
        };

        enum HttpMethod
        {
            kGet = 0,
            kPost,
            kHead,
            kPut,
            kDelete,
            kOptions,
            kPatch,
            kInvalid
        };
    }
}