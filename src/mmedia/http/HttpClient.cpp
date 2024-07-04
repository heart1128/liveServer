#include "HttpClient.h"
#include "mmedia/http/HttpContext.h"
#include "mmedia/base/MMediaLog.h"
#include "network/DnsServer.h"

using namespace tmms::mm;
using namespace tmms::network;

HttpClient::HttpClient(EventLoop *loop, HttpHandler *handler)
 :loop_(loop), handler_(handler)
{
}

HttpClient::~HttpClient()
{
}

void HttpClient::OnWriteComplete(const TcpConnectionPtr & conn)
{
    auto context = conn->GetContext<HttpContext>(kHttpContext);
    if(context)
    {
        // 写完成，判断之后发起http connect
        context->WriteComplete(conn);
    }
}

/// @brief 连接之后，创建conetext
/// @param conn 
/// @param connected 
void HttpClient::OnConnection(const TcpConnectionPtr &conn, bool connected)
{ 
    if(connected)
    {
        // 创建客户端context，设置推流或者拉流，最后保存到conn的上下文
        auto context = std::make_shared<HttpContext>(conn, handler_);

        conn->SetContext(kHttpContext, context);
        if(is_post_)
        {
            context->PostRequest(request_->MakeHeaders(), out_packet_);
        }
        else
        {
            // get没有body
            context->PostRequest(request_->MakeHeaders());
        }

    }
}

/// @brief 客户端收到了Http服务端发送的消息，进行消息解析
/// @param conn 
/// @param buf 
void HttpClient::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    auto context = conn->GetContext<HttpContext>(kHttpContext);
    if(context)
    {
        auto ret = context->Parse(buf);
        if(ret == -1)
        {
            RTMP_ERROR << "httpClient: message parse error.";
            conn->ForceClose();
        }
    }
}

bool HttpClient::ParseUrl(const std::string &url)
{
    if(url.size() > 7) // http://
    {
        uint16_t port = 80; // 如果url没有带端口，默认80
        int idx = 7;
        if(url[idx] == '/')  // https://
        {
            idx++;
        }
        auto pos = url.find_first_of("/", idx); // http://heart.com/live/test
        if(pos != std::string::npos)
        {
            // domain之后就是一个请求路径
            const std::string &path = url.substr(pos);
            auto pos1 = path.find_first_of("?"); // 有问号就是带参数的
            if(pos1 != std::string::npos)
            {
                request_->SetPath(path.substr(0,pos1));
                request_->SetQuery(path.substr(pos1 + 1));
            }
            else // 没有参数，是整个路径
            {
                request_->SetPath(path);
            }

            std::string domain = url.substr(idx, pos - idx);
            request_->AddHeader("Host", domain);

            auto pos2 = domain.find_first_of(":"); // 找端口
            if(pos2 != std::string::npos)  // 在这个位置有冒号，就是带port的
            {
                addr_.SetAddr(domain.substr(0, pos2)); // host
                addr_.SetPort(std::atoi(url.substr(pos2 + 1).c_str())); //port
            }
            else // 只有host
            {
                addr_.SetAddr(domain);
                addr_.SetPort(port);
            }
            return true;
        }
        else  // http://heart.com
        {
            request_->SetPath("/"); // 没有请求的路径。设置为根目录
            std::string domain = url.substr(7);
            request_->AddHeader("Host", domain);
            addr_.SetAddr(domain);
            addr_.SetPort(port);
        }
        // domain需要dns转换
        // 域名找ip，域名可能对应多个ip，找一个ipv4的
        auto list = sDnsService->GetHostAddress(addr_.IP());
        if(list.size() > 0)
        {
            for(auto const &l : list)
            {
                // 找一个ipv4的地址
                if(!l->IsIpV6())
                {
                    addr_.SetAddr(l->IP());
                    break;
                }
            }
        }
        else  // 没找到直接通过ip,增加
        {
            sDnsService->AddHost(addr_.IP()); // 下一次就能直接找到了
            std::vector<InetAddress::ptr> list2;
            sDnsService->GetHostInfo(addr_.IP(), list2);
            for(auto const &l : list2)
            {
                // 找一个ipv4的地址
                if(!l->IsIpV6())
                {
                    addr_.SetAddr(l->IP());
                    break;
                }
            }
        }
        return true;
    }
    return false;
}

/// @brief 创建tcpClient，绑定回调，启动连接
void HttpClient::CreateTcpClient()
{
    request_.reset();
    request_ = std::make_shared<HttpRequest>(true);
    // 设置请求方法
    if(is_post_)
    {
        request_->SetMethod(kPost);
    }
    else
    {
        request_->SetMethod(kGet);
    }
    request_->AddHeader("User-Agent","curl/7.61.1");
    request_->AddHeader("Accept","*/*");
    auto ret = ParseUrl(url_);
    if(!ret)
    {
        RTMP_TRACE << "invalid url:" << url_;
        RTMP_ERROR << "invalid url:" << url_;
        if(close_cb_)
        {
            close_cb_(nullptr);
        }
        return;
    }
    // 创建tcpClient，绑定回调
    tcp_client_ = std::make_shared<TcpClient>(loop_, addr_);
    tcp_client_->SetWriteCompleteCallback(std::bind(&HttpClient::OnWriteComplete, this, std::placeholders::_1));
    tcp_client_->SetRecvMsgCallback(std::bind(&HttpClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcp_client_->SetCloseCallback(close_cb_);
    tcp_client_->SetConnectCallback(std::bind(&HttpClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));
    tcp_client_->Connect();
}

void HttpClient::SetCloseCallback(const CloseConnectionCallback &cb)
{
    close_cb_ = cb;
}

void HttpClient::SetCloseCallback(CloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);
}

void HttpClient::Get(const std::string &url)
{
    is_post_ = false;
    url_ = url;
    CreateTcpClient();
}

void HttpClient::Post(const std::string &url, const PacketPtr &packet)
{
    is_post_ = true;
    url_ = url;
    out_packet_ = packet;
    CreateTcpClient();
}

