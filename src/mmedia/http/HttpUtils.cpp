/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-02 13:32:07
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-02 13:32:10
 * @FilePath: /liveServer/src/mmedia/http/HttpUtils.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%A
 */
#include "HttpUtils.h"
#include "HttpTypes.h"
#include <unordered_map>
namespace
{
    static std::string empty_string;
}
using namespace tmms::mm;

HttpStatusCode HttpUtils::ParseStatusCode(int32_t code)
{
    static std::unordered_map<int32_t,HttpStatusCode> maps = 
    {
        {0,kUnknown},
        {100,k100Continue},
        {101,k101SwitchingProtocols},
        {200,k200OK},
        {201,k201Created},
        {202,k202Accepted},
        {203,k203NonAuthoritativeInformation},
        {204,k204NoContent},
        {205,k205ResetContent},
        {206,k206PartialContent},
        {300,k300MultipleChoices},
        {301,k301MovedPermanently},
        {302,k302Found},
        {303,k303SeeOther},
        {304,k304NotModified},
        {305,k305UseProxy},
        {307,k307TemporaryRedirect},
        {308,k308PermanentRedirect},
        {400,k400BadRequest},
        {401,k401Unauthorized},
        {402,k402PaymentRequired},
        {403,k403Forbidden},
        {404,k404NotFound},
        {405,k405MethodNotAllowed},
        {406,k406NotAcceptable},
        {407,k407ProxyAuthenticationRequired},
        {408,k408RequestTimeout},
        {409,k409Conflict},
        {410,k410Gone},
        {411,k411LengthRequired},
        {412,k412PreconditionFailed},
        {413,k413RequestEntityTooLarge},
        {414,k414RequestURITooLarge},
        {415,k415UnsupportedMediaType},
        {416,k416RequestedRangeNotSatisfiable},
        {417,k417ExpectationFailed},
        {418,k418ImATeapot},
        {421,k421MisdirectedRequest},
        {425,k425TooEarly},
        {426,k426UpgradeRequired},
        {428,k428PreconditionRequired},
        {429,k429TooManyRequests},
        {431,k431RequestHeaderFieldsTooLarge},
        {451,k451UnavailableForLegalReasons},
        {500,k500InternalServerError},
        {501,k501NotImplemented},
        {502,k502BadGateway},
        {503,k503ServiceUnavailable},
        {504,k504GatewayTimeout},
        {505,k505HTTPVersionNotSupported},
        {510,k510NotExtended},
    };
    auto iter = maps.find(code);
    if(iter!= maps.end())
    {
        return iter->second;
    }
    return kUnknown;
}

std::string HttpUtils::ParseStatusMessage(int32_t code)
{
    static std::unordered_map<int32_t,std::string> maps = 
    {
        {0,"unknown"},
        {100,"Continue"},
        {101,"Switching Protocols"},
        {200,"OK"},
        {201,"Created"},
        {202,"Accepted"},
        {203,"Non Authoritative Information"},
        {204,"No Content"},
        {205,"Reset Content"},
        {206,"Partial Content"},
        {300,"Multiple Choices"},
        {301,"Moved Permanently"},
        {302,"Found"},
        {303,"See Other"},
        {304,"Not Modified"},
        {305,"Use Proxy"},
        {307,"Temporary Redirect"},
        {308,"Permanent Redirect"},
        {400,"Bad Request"},
        {401,"Unauthorized"},
        {402,"Payment Required"},
        {403,"Forbidden"},
        {404,"Not Found"},
        {405,"Method Not Allowed"},
        {406,"Not Acceptable"},
        {407,"Proxy Authentication Required"},
        {408,"Request Timeout"},
        {409,"Conflict"},
        {410,"Gone"},
        {411,"Length Required"},
        {412,"Precondition Failed"},
        {413,"Request Entity Too Large"},
        {414,"Request URI Too Large"},
        {415,"Unsupported Media Type"},
        {416,"Requested Range Not Satisfiable"},
        {417,"Expectation Failed"},
        {418,"Im A Teapot"},
        {421,"Misdirected Request"},
        {425,"Too Early"},
        {426,"Upgrade Required"},
        {428,"Precondition Required"},
        {429,"Too Many Requests"},
        {431,"Request Header Fields Too Large"},
        {451,"Unavailable For Legal Reasons"},
        {500,"Internal Server Error"},
        {501,"Not Implemented"},
        {502,"Bad Gateway"},
        {503,"Service Unavailable"},
        {504,"Gateway Timeout"},
        {505,"HTTP Version Not Supported"},
        {510,"Not Extended"},
    };
    auto iter = maps.find(code);
    if(iter!= maps.end())
    {
        return iter->second;
    }
    return "unknown";
}

HttpMethod HttpUtils::ParseMethod(const std::string &method)
{
    static std::unordered_map<std::string,HttpMethod> maps = 
    {
        {"GET",kGet},
        {"PUT",kPut},
        {"POST",kPost},
        {"HEAD",kHead},
        {"DELETE",kDelete},
        {"OPTIONS",kOptions}
    };
    auto it = maps.find(method);
    if(it!= maps.end())
    {
        return it->second;
    }
    return kInvalid;
}            
const std::string &HttpUtils::ContentTypeToString(ContentType contenttype)
{
    switch (contenttype)
    {
        case kContentTypeTextHTML:
        {
            static std::string sv =
                "Content-Type: text/html; charset=utf-8\r\n";
            return sv;
        }
        case kContentTypeAppXForm:
        {
            static std::string sv =
                "Content-Type: application/x-www-form-urlencoded\r\n";
            return sv;
        }
        case kContentTypeTextXML:
        {
            static std::string sv =
                "Content-Type: text/xml; charset=utf-8\r\n";
            return sv;
        }
        case kContentTypeAppXML:
        {
            static std::string sv =
                "Content-Type: application/xml; charset=utf-8\r\n";
            return sv;
        }            
        case kContentTypeAppJson:
        {
            static std::string sv =
                "Content-Type: application/json; charset=utf-8\r\n";
            return sv;
        }
        case kContentTypeAppMpegUrl:
        {
            static std::string sv = "Content-Type: application/vnd.apple.mpegurl\r\n";
            return sv;
        }
        case kContentTypeVideoMP2T:
        {
            static std::string sv = "Content-Type: video/MP2T\r\n";
            return sv;
        }
        case kContentTypeVideoXFlv:
        {
            static std::string sv = "Content-Type: video/x-flv\r\n";
            return sv;
        }            
        case kContentTypeNONE:
        {
            static std::string sv = "";
            return sv;
        }
        default:
        case kContentTypeTextPlain:
        {
            static std::string sv =
                "Content-Type: text/plain; charset=utf-8\r\n";
            return sv;
        }
    }
}

const std::string &HttpUtils::StatusCodeToString(int code)
{
    switch (code)
    {
        case 100:
        {
            static std::string sv = "Continue";
            return sv;
        }
        case 101:
        {
            static std::string sv = "Switching Protocols";
            return sv;
        }
        case 200:
        {
            static std::string sv = "OK";
            return sv;
        }
        case 201:
        {
            static std::string sv = "Created";
            return sv;
        }
        case 202:
        {
            static std::string sv = "Accepted";
            return sv;
        }
        case 203:
        {
            static std::string sv = "Non-Authoritative Information";
            return sv;
        }
        case 204:
        {
            static std::string sv = "No Content";
            return sv;
        }
        case 205:
        {
            static std::string sv = "Reset Content";
            return sv;
        }
        case 206:
        {
            static std::string sv = "Partial Content";
            return sv;
        }
        case 300:
        {
            static std::string sv = "Multiple Choices";
            return sv;
        }
        case 301:
        {
            static std::string sv = "Moved Permanently";
            return sv;
        }
        case 302:
        {
            static std::string sv = "Found";
            return sv;
        }
        case 303:
        {
            static std::string sv = "See Other";
            return sv;
        }
        case 304:
        {
            static std::string sv = "Not Modified";
            return sv;
        }
        case 305:
        {
            static std::string sv = "Use Proxy";
            return sv;
        }
        case 307:
        {
            static std::string sv = "Temporary Redirect";
            return sv;
        }
        case 308:
        {
            static std::string sv = "Permanent Redirect";
            return sv;
        }
        case 400:
        {
            static std::string sv = "Bad Request";
            return sv;
        }
        case 401:
        {
            static std::string sv = "Unauthorized";
            return sv;
        }
        case 402:
        {
            static std::string sv = "Payment Required";
            return sv;
        }
        case 403:
        {
            static std::string sv = "Forbidden";
            return sv;
        }
        case 404:
        {
            static std::string sv = "Not Found";
            return sv;
        }
        case 405:
        {
            static std::string sv = "Method Not Allowed";
            return sv;
        }
        case 406:
        {
            static std::string sv = "Not Acceptable";
            return sv;
        }
        case 407:
        {
            static std::string sv = "Proxy Authentication Required";
            return sv;
        }
        case 408:
        {
            static std::string sv = "Request Time-out";
            return sv;
        }
        case 409:
        {
            static std::string sv = "Conflict";
            return sv;
        }
        case 410:
        {
            static std::string sv = "Gone";
            return sv;
        }
        case 411:
        {
            static std::string sv = "Length Required";
            return sv;
        }
        case 412:
        {
            static std::string sv = "Precondition Failed";
            return sv;
        }
        case 413:
        {
            static std::string sv = "Request Entity Too Large";
            return sv;
        }
        case 414:
        {
            static std::string sv = "Request-URI Too Large";
            return sv;
        }
        case 415:
        {
            static std::string sv = "Unsupported Media Type";
            return sv;
        }
        case 416:
        {
            static std::string sv = "Requested Range Not Satisfiable";
            return sv;
        }
        case 417:
        {
            static std::string sv = "Expectation Failed";
            return sv;
        }
        case 418:
        {
            static std::string sv = "I'm a Teapot";
            return sv;
        }
        case 421:
        {
            static std::string sv = "Misdirected Request";
            return sv;
        }
        case 425:
        {
            static std::string sv = "Too Early";
            return sv;
        }
        case 426:
        {
            static std::string sv = "Upgrade Required";
            return sv;
        }
        case 428:
        {
            static std::string sv = "Precondition Required";
            return sv;
        }
        case 429:
        {
            static std::string sv = "Too Many Requests";
            return sv;
        }
        case 431:
        {
            static std::string sv = "Request Header Fields Too Large";
            return sv;
        }
        case 451:
        {
            static std::string sv = "Unavailable For Legal Reasons";
            return sv;
        }
        case 500:
        {
            static std::string sv = "Internal Server Error";
            return sv;
        }
        case 501:
        {
            static std::string sv = "Not Implemented";
            return sv;
        }
        case 502:
        {
            static std::string sv = "Bad Gateway";
            return sv;
        }
        case 503:
        {
            static std::string sv = "Service Unavailable";
            return sv;
        }
        case 504:
        {
            static std::string sv = "Gateway Time-out";
            return sv;
        }
        case 505:
        {
            static std::string sv = "HTTP Version Not Supported";
            return sv;
        }
        case 510:
        {
            static std::string sv = "Not Extended";
            return sv;
        }
        default:
            if (code >= 100 && code < 200)
            {
                static std::string sv = "Informational";
                return sv;
            }
            else if (code >= 200 && code < 300)
            {
                static std::string sv = "Successful";
                return sv;
            }
            else if (code >= 300 && code < 400)
            {
                static std::string sv = "Redirection";
                return sv;
            }
            else if (code >= 400 && code < 500)
            {
                static std::string sv = "Bad Request";
                return sv;
            }
            else if (code >= 500 && code < 600)
            {
                static std::string sv = "Server Error";
                return sv;
            }
            else
            {
                static std::string sv = "Undefined Error";
                return sv;
            }
    }
    return empty_string;
}

ContentType HttpUtils::GetContentType(const std::string &fileName)
{
    std::string extName;
    auto pos = fileName.rfind('.');
    if (pos != std::string::npos)
    {
        extName = fileName.substr(pos + 1);
        std::transform(extName.begin(), extName.end(), extName.begin(), tolower);
    }
    switch (extName.length())
    {
        case 0:
            return kContentTypeNONE;
        case 2:
        {
            if (extName == "ts")
                return kContentTypeVideoMP2T;
        }
        case 3:
        {
            if (extName == "flv")
                return kContentTypeVideoXFlv;
        }
        case 4:
        {
            if (extName == "html")
                return kContentTypeTextHTML;
        }
        default:
            return kContentTypeTextPlain;
    }
}

ContentType HttpUtils::ParseContentType(const std::string &contentType)
{
    static const std::unordered_map<std::string, ContentType> map_{
        {"text/html", kContentTypeTextHTML},
        {"application/x-www-form-urlencoded", kContentTypeAppXForm},
        {"application/xml", kContentTypeAppXML},
        {"application/json", kContentTypeAppJson},
        {"text/plain", kContentTypeTextPlain}};
    auto iter = map_.find(contentType);
    if (iter == map_.end())
        return kContentTypeNONE;
    return iter->second;
}

std::string HttpUtils::CharToHex(char c)
{
    std::string result;
    char first, second;

    first = (c & 0xF0) >> 4;
    first += first > 9 ? 'A' - 10 : '0';
    second = c & 0x0F;
    second += second > 9 ? 'A' - 10 : '0';

    result.append(1, first);
    result.append(1, second);

    return result;
}
std::string HttpUtils::UrlEncode(const std::string &src)
{
    std::string result;
    std::string::const_iterator iter;

    for (iter = src.begin(); iter != src.end(); ++iter)
    {
        switch (*iter)
        {
            case ' ':
                result.append(1, '+');
                break;
            // alnum
            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G':
            case 'H':
            case 'I':
            case 'J':
            case 'K':
            case 'L':
            case 'M':
            case 'N':
            case 'O':
            case 'P':
            case 'Q':
            case 'R':
            case 'S':
            case 'T':
            case 'U':
            case 'V':
            case 'W':
            case 'X':
            case 'Y':
            case 'Z':
            case 'a':
            case 'b':
            case 'c':
            case 'd':
            case 'e':
            case 'f':
            case 'g':
            case 'h':
            case 'i':
            case 'j':
            case 'k':
            case 'l':
            case 'm':
            case 'n':
            case 'o':
            case 'p':
            case 'q':
            case 'r':
            case 's':
            case 't':
            case 'u':
            case 'v':
            case 'w':
            case 'x':
            case 'y':
            case 'z':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            // mark
            case '-':
            case '_':
            case '.':
            case '!':
            case '~':
            case '*':
            case '\'':
            case '(':
            case ')':
            case '&':
            case '=':
            case '/':
            case '\\':
            case '?':
                result.append(1, *iter);
                break;
            // escape
            default:
                result.append(1, '%');
                result.append(CharToHex(*iter));
                break;
        }
    }

    return result;
}        
bool HttpUtils::NeedUrlDecoding(const std::string &url)
{
    const char * start = url.data(); 
    const char * end = start + url.size();
    
    return std::find_if(start, end, [](const char c) {
            return c == '+' || c == '%';
        }) != end;
}
std::string HttpUtils::UrlDecode(const std::string &url)
{
    std::string result;
    size_t len = url.size();
    result.reserve(len * 2);
    int hex = 0;
    for (size_t i = 0; i < len; ++i)
    {
        switch (url[i])
        {
            case '+':
                result += ' ';
                break;
            case '%':
                if ((i + 2) < len && isxdigit(url[i + 1]) &&
                    isxdigit(url[i + 2]))
                {
                    unsigned int x1 = url[i + 1];
                    if (x1 >= '0' && x1 <= '9')
                    {
                        x1 -= '0';
                    }
                    else if (x1 >= 'a' && x1 <= 'f')
                    {
                        x1 = x1 - 'a' + 10;
                    }
                    else if (x1 >= 'A' && x1 <= 'F')
                    {
                        x1 = x1 - 'A' + 10;
                    }
                    unsigned int x2 = url[i + 2];
                    if (x2 >= '0' && x2 <= '9')
                    {
                        x2 -= '0';
                    }
                    else if (x2 >= 'a' && x2 <= 'f')
                    {
                        x2 = x2 - 'a' + 10;
                    }
                    else if (x2 >= 'A' && x2 <= 'F')
                    {
                        x2 = x2 - 'A' + 10;
                    }
                    hex = x1 * 16 + x2;

                    result += char(hex);
                    i += 2;
                }
                else
                {
                    result += '%';
                }
                break;
            default:
                result += url[i];
                break;
        }
    }
    return result;
} 
        
