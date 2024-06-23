#include "RtmpClient.h"
#include "mmedia/rtmp/RtmpContext.h"
#include "mmedia/base/MMediaLog.h"

using namespace tmms::mm;
using namespace tmms::network;

RtmpClient::RtmpClient(EventLoop *loop, RtmpHandler *handler)
 :loop_(loop), handler_(handler)
{
}

RtmpClient::~RtmpClient()
{
}

void RtmpClient::OnWriteComplete(const TcpConnectionPtr & conn)
{
    auto context = conn->GetContext<RtmpContext>(kRtmpContext);
    if(context)
    {
        // 写完成，判断之后发起rtmp connect
        context->OnWriteComplete();
    }
}

/// @brief 连接之后，创建conetext
/// @param conn 
/// @param connected 
void RtmpClient::OnConnection(const TcpConnectionPtr &conn, bool connected)
{ 
    if(connected)
    {
        // 创建客户端context，设置推流或者拉流，最后保存到conn的上下文
        auto context = std::make_shared<RtmpContext>(conn, handler_, true);
        if(is_player_)
        {
            context->Play(url_);
        }
        else
        {
            context->Publish(url_);
        }
        conn->SetContext(kRtmpContext, context);
        // 启动客户端握手
        context->StartHandShake();
    }
}

/// @brief 客户端收到了Rtmp服务端发送的消息，进行消息解析
/// @param conn 
/// @param buf 
void RtmpClient::OnMessage(const TcpConnectionPtr &conn, MsgBuffer &buf)
{
    auto context = conn->GetContext<RtmpContext>(kRtmpContext);
    if(context)
    {
        auto ret = context->Parse(buf);
        if(ret == -1)
        {
            RTMP_ERROR << "rtmpClient: message parse error.";
            conn->ForceClose();
        }
    }
}

bool RtmpClient::ParseUrl(const std::string &url)
{
    if(url.size() > 7) // rtmp://
    {
        uint16_t port = 1935; // 如果url没有带端口，默认1935
        auto pos = url.find_first_of(":/", 7); // rtmp://10.144.0.1/live/test
        if(pos != std::string::npos)
        {
            std::string domain = url.substr(7, pos - 7);
            if(url.at(pos) == ':')  // 在这个位置有冒号，就是带port的
            {
                auto pos1 = url.find_first_of("/", pos + 1); // 从:port/到下一个字段
                if(pos1 != std::string::npos)
                {
                    port = std::atoi(url.substr(pos + 1, pos1 - pos).c_str());
                }
            }
            addr_.SetAddr(domain);
            RTMP_TRACE << "domain:" << domain << std::endl;
            addr_.SetPort(port);
            return true;
        }
    }
    return false;
}

/// @brief 创建tcpClient，绑定回调，启动连接
void RtmpClient::CreateTcpClient()
{
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
    tcp_client_->SetWriteCompleteCallback(std::bind(&RtmpClient::OnWriteComplete, this, std::placeholders::_1));
    tcp_client_->SetRecvMsgCallback(std::bind(&RtmpClient::OnMessage, this, std::placeholders::_1, std::placeholders::_2));
    tcp_client_->SetCloseCallback(close_cb_);
    tcp_client_->SetConnectCallback(std::bind(&RtmpClient::OnConnection, this, std::placeholders::_1, std::placeholders::_2));
    tcp_client_->Connect();
}

void RtmpClient::SetCloseCallback(const CloseConnectionCallback &cb)
{
    close_cb_ = cb;
}

void RtmpClient::SetCloseCallback(CloseConnectionCallback &&cb)
{
    close_cb_ = std::move(cb);
}

void RtmpClient::Play(const std::string &url)
{
    is_player_ = true;
    url_ = url;
    CreateTcpClient();
}

void RtmpClient::Publish(const std::string &url)
{
    is_player_ = false;
    url_ = url;
    CreateTcpClient();
}

void RtmpClient::Send(PacketPtr &&data)
{
}
