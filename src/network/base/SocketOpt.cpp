/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 14:18:13
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-05 16:01:40
 * @FilePath: /tmms/src/network/base/SocketOpt.cpp
 * @Description:  learn 
 */
#include "SocketOpt.h"
#include "Network.h"

using namespace tmms::base;

SocketOpt::SocketOpt(int sock, bool v6)
:sock_(sock), is_v6_(v6)
{
}

/// @brief 创建非阻塞的tcp socket
/// @param family 
/// @return sockfd
int SocketOpt::CreateNonblockingTcpSocket(int family)
{
    /// SOCK_CLOEXEC 可确保在调用 exec 家族函数（如 execve()）时，套接字的文件描述符将自动关闭。
    // 这可以防止在使用多进程或执行其他程序的情况下发生文件描述符泄漏。
    int sock = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if(sock < 0)
    {
        NETWORK_ERROR << "socket failed ! ";
    }
    return sock;
}

/// @brief 创建非阻塞的udp socket
/// @param family 
/// @return scokfd
int SocketOpt::CreateNonblockingUdpSocket(int family)
{
    int sock = ::socket(family, SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_UDP);
    if(sock < 0)
    {
        NETWORK_ERROR << "socket failed ! ";
    }
    return sock;
}

int SocketOpt::BindAddress(const InetAdress &localaddr)
{
    if(localaddr.IsIpV6())      // ipv6
    {
        struct sockaddr_in6 addr;
        localaddr.GetSockAddr((struct sockaddr*)&addr);
        socklen_t size = sizeof(addr);
        return ::bind(sock_,(struct sockaddr*)&addr, size);
    }

    // ipv4
    struct sockaddr_in addr;
    localaddr.GetSockAddr((struct sockaddr*)&addr);
    socklen_t size = sizeof(addr);
    return ::bind(sock_, (struct sockaddr*)&addr, size);
}

int SocketOpt::Listen()
{
    return ::listen(sock_, SOMAXCONN);
}

/// @brief socket_accept
/// @param peeraddr  传入接收的地址，接收一个accept之后的客户端地址
/// @return 返回客户端的文件描述符，-1就是失败了
int SocketOpt::Accept(InetAdress *peeraddr)
{
    struct sockaddr_in6 addr;   // 可能接受的是ipv4或者ipv6的，用大的容量装再判断
    socklen_t size = sizeof(addr);
    int sock = ::accept4(sock_, (struct sockaddr*)&addr, &size, SOCK_CLOEXEC | SOCK_NONBLOCK); // 子进程关闭父进程

    if(sock > 0)
    {
        // 如果协议族是ipv4的，就进行结构体装换
        if(addr.sin6_family == AF_INET)
        {
            char ip[16] = {0,};
            struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&addr;
            // 网络端的ip转换成host端的
            ::inet_ntop(AF_INET, &(addr_v4->sin_addr.s_addr), ip, sizeof(ip));

            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr_v4->sin_port));
        }
        else if(addr.sin6_family == AF_INET6)
        {
            char ip[INET6_ADDRSTRLEN] = {0,};
            ::inet_ntop(AF_INET6, &(addr.sin6_addr), ip, sizeof(ip));

            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr.sin6_port));
            peeraddr->SetIsIPv6(true);
        }
    }
    return sock;
}

int SocketOpt::Connect(const InetAdress &addr)
{
    struct sockaddr_in6 addr_in;
    addr.GetSockAddr((struct sockaddr*)&addr_in);

    return ::connect(sock_, (struct sockaddr*)&addr_in, sizeof(struct sockaddr_in6));
}

/// @brief 获取本地地址
/// @param saddr 
/// @return 一个InetAddress的智能指针
InetAdress::ptr SocketOpt::GetLocalAddr()
{
    struct sockaddr_in6 addr_in;
    socklen_t len = sizeof(struct sockaddr_in6);
    // 这个函数获取本地的地址和地址长度
    ::getsockname(sock_,(struct sockaddr*)&addr_in, &len);
    
    InetAdress::ptr localaddr = std::make_shared<InetAdress>();
    // 如果协议族是ipv4的，就进行结构体装换
    if(addr_in.sin6_family == AF_INET)
    {
        char ip[16] = {0,};
        struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&addr_in;
        // 网络端的ip转换成host端的
        ::inet_ntop(AF_INET, &(addr_v4->sin_addr.s_addr), ip, sizeof(ip));

        localaddr->SetAddr(ip);
        localaddr->SetPort(ntohs(addr_v4->sin_port));
        localaddr->SetIsIPv6(false);
    }
    else if(addr_in.sin6_family == AF_INET6)
    {
        char ip[INET6_ADDRSTRLEN] = {0,};
        // 转换成点分十进制的字符串
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));

        localaddr->SetAddr(ip);
        localaddr->SetPort(ntohs(addr_in.sin6_port));
        localaddr->SetIsIPv6(true);
    }

    return localaddr;
}

/// @brief 获取远端地址
/// @param saddr 
/// @return 一个InetAddress的智能指针
InetAdress::ptr SocketOpt::GetPeerAddr()
{
    struct sockaddr_in6 addr_in;
    socklen_t len = sizeof(struct sockaddr_in6);
    // 这个函数获取远端的地址和地址长度
    ::getpeername(sock_,(struct sockaddr*)&addr_in, &len);
    
    InetAdress::ptr localaddr = std::make_shared<InetAdress>();
    // 如果协议族是ipv4的，就进行结构体装换
    if(addr_in.sin6_family == AF_INET)
    {
        char ip[16] = {0,};
        struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&addr_in;
        // 网络端的ip转换成host端的
        ::inet_ntop(AF_INET, &(addr_v4->sin_addr.s_addr), ip, sizeof(ip));

        localaddr->SetAddr(ip);
        localaddr->SetPort(ntohs(addr_v4->sin_port));
        localaddr->SetIsIPv6(false);
    }
    else if(addr_in.sin6_family == AF_INET6)
    {
        char ip[INET6_ADDRSTRLEN] = {0,};
        // 转换成点分十进制的字符串
        ::inet_ntop(AF_INET6, &(addr_in.sin6_addr), ip, sizeof(ip));

        localaddr->SetAddr(ip);
        localaddr->SetPort(ntohs(addr_in.sin6_port));
        localaddr->SetIsIPv6(true);
    }

    return localaddr;
}

/// @brief 是否禁用Nagle算法
/// @param on 
void SocketOpt::SetTcpNoDelay(bool on)
{
    int optvalue = on ? 1 : 0;
    // sock, tcp协议，sockopt 无延迟
    // TCP_NODELAY指定是否禁用Nagle算法的选项名称。当启用此选项（设置为 1）时，它禁用Nagle算法，这意味着数据会尽快发送而不等待填满TCP数据包。
    ::setsockopt(sock_, IPPROTO_TCP, TCP_NODELAY, &optvalue, sizeof(optvalue));
}

/// @brief 设置地址重用，服务器在bind的时候使用，time_wait的时候就可以直接使用
/// @param on 
void SocketOpt::SetReuseAddr(bool on)
{
    int optvalue = on ? 1 : 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEADDR, &optvalue, sizeof(optvalue));
}

/// @brief 设置端口复用，客户端在connection的时候使用，因为服务端端口不变
/// @param on 
void SocketOpt::SetReusePort(bool on)
{
    int optvalue = on ? 1 : 0;
    ::setsockopt(sock_, SOL_SOCKET, SO_REUSEPORT, &optvalue, sizeof(optvalue));
}

/// @brief 设置非阻塞socket，非阻塞套接字在进行I/O操作时不会阻塞调用线程。
/// @param on 
void SocketOpt::SetNonBlocking(bool on)
{
    int flag = ::fcntl(sock_, F_GETFL, 0);
    if(on)
    {
        flag |= O_NONBLOCK;
    }
    else
    {
        flag &= ~O_NONBLOCK;
    }
    ::fcntl(sock_, F_SETFL, flag);
}
