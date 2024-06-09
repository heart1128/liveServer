/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-08 21:09:09
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 08:49:47
 * @FilePath: /tmms/src/network/net/DnsServer.cpp
 * @Description:  learn 
 */
#include "DnsServer.h"
#include <functional>
#include <cstring>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

using namespace tmms::network;

namespace
{
    static InetAddress::ptr inet_address_null;
}

/// @brief 添加host和ip地址的映射对，如果有就直接返回，没有就插入一个空的vector
/// @param host  域名
void DnsServer::AddHost(const std::string &host)
{
    std::lock_guard<std::mutex> lk(lock_);

    auto iter = hosts_info_.find(host);
    if(iter != hosts_info_.end())
    {
        return;
    }

    hosts_info_[host] = std::vector<InetAddress::ptr>();
}

/// @brief 获取域名对应的ip地址类
/// @param host 
/// @param index 
/// @return 对应的ip地址类，如果没有找到，返回一个空的智能指针
InetAddress::ptr DnsServer::GetHostAddress(const std::string &host, int index)
{
    std::lock_guard<std::mutex> lk(lock_);
    
    auto iter = hosts_info_.find(host);
    if(iter != hosts_info_.end())
    {
        auto list = iter->second;
        if(list.size() > 0)
        {
            return list[index % list.size()];
        }
    }

    return inet_address_null;
}

/// @brief 一个域名可能对应多个ip地址，取所有的对应ip地址
/// @param host 
/// @return 一个ip地址的数组
std::vector<InetAddress::ptr> DnsServer::GetHostAddress(const std::string &host)
{
    std::lock_guard<std::mutex> lk(lock_);
    
    auto iter = hosts_info_.find(host);
    if(iter != hosts_info_.end())
    {
        return iter->second;
    }

    return std::vector<InetAddress::ptr>(); 
}

/// @brief 替换整个域名列表
/// @param host 
/// @param list 
void DnsServer::UpdateHost(const std::string &host, std::vector<InetAddress::ptr> &list)
{
    std::lock_guard<std::mutex> lk(lock_);
    hosts_info_[host].swap(list);
}

/// @brief 返回整个域名表
/// @return 返回整个域名表
std::unordered_map<std::string, std::vector<InetAddress::ptr>> DnsServer::GetHosts()
{
    std::lock_guard<std::mutex> lk(lock_);
    return hosts_info_;
}

/// @brief 设置dns的参数，
/// @param interval 
/// @param sleep 
/// @param retry 
void DnsServer::SetDnsServiceParam(int32_t interval, int32_t sleep, int32_t retry)
{
    interval_ = interval;
    sleep_ = sleep;
    retry_ = retry;
}

void DnsServer::Start()
{
    running_ = true;
    thread_ = std::thread(std::bind(&DnsServer::OnWork, this));
}

void DnsServer::Stop()
{
    running_ = false;
    if(thread_.joinable())
    {
        thread_.join();
    }
}

void DnsServer::OnWork()
{
    while(running_)
    {
        auto host_infos = GetHosts();

        for(auto &host : hosts_info_)
        {
            // 可能获取失败，有重试次数
            for(int i = 0; i < retry_; i++)
            {
                std::vector<InetAddress::ptr> list;
                GetHostInfo(host.first, list);
                if(list.size() > 0) //获取成功了就更新hosts_list_,跳出
                {
                    UpdateHost(host.first, list);
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(sleep_));
            }
        }
        // 每个间隔都要更新一次
        std::this_thread::sleep_for(std::chrono::milliseconds(interval_));
    }
}

/// @brief 利用getaddrinfo的系统调用，通过host获取对应的ip信息到结构体中，然后进行解析转换
/// @param host 
/// @param list 
void DnsServer::GetHostInfo(const std::string &host, std::vector<InetAddress::ptr> &list)
{
    struct addrinfo ainfo, *res;
    memset(&ainfo, 0x00, sizeof(ainfo));
    ainfo.ai_family = AF_UNSPEC;    // 不指定协议族，不指定是IPv4（AF_INET）还是IPv6（AF_INET6）地址。允许两种类型的地址。
    ainfo.ai_flags = AI_PASSIVE;    //表示返回的套接字地址将用于调用bind()。如果node参数（主机名）为NULL，每个sockaddr结构体中的IP地址将设置为INADDR_ANY（对于IPv4地址）或IN6ADDR_ANY_INIT（对于IPv6地址）
    ainfo.ai_socktype = SOCK_DGRAM;// 套接字类型为数据报（UDP）
    /*
        getaddrinfo 是一个在网络编程中用于域名解析的函数。它的主要作用是将主机名和服务名转换为套接字地址结构，也就是struct sockadd_in/6
        这些结构可以直接用于 socket、connect、bind 等函数。这个函数在许多现代操作系统中都可用，并支持IPv4和IPv6
    */
    // 参数1：主机名或IP地址, 如果host为空字符串或NULL，表示地址应设置为INADDR_ANY或IN6ADDR_ANY_INIT，具体取决于协议族。
    // 参数2：指定服务名和端口号，nullptr未指定服务名或端口号
    // 参数3：指向带有条件的addrinfo结构体的指针。
    // 参数4：指向一个指针的指针，结果将存储在这里。
    // 如果getaddrinfo返回-1或res为nullptr，表示出现错误，函数直接返回。
    auto ret = ::getaddrinfo(host.c_str(), nullptr, &ainfo, &res);
    if(ret == -1 || res == nullptr)
    {
        return;
    } 

    // 解析地址，一个域名可能多应多个套接字地址
    struct addrinfo *rp = res;
    for(;rp != nullptr; rp = rp->ai_next)
    {
        InetAddress::ptr peeraddr = std::make_shared<InetAddress>();
        // 如果协议族是ipv4的，就进行结构体装换
        if(rp->ai_family == AF_INET)
        {
            char ip[16] = {0,};
            struct sockaddr_in* addr_v4 = (struct sockaddr_in*)&rp->ai_addr;
            // 网络端的ip转换成host端的
            ::inet_ntop(AF_INET, &(addr_v4->sin_addr.s_addr), ip, sizeof(ip));

            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr_v4->sin_port));
        }
        else if(rp->ai_family == AF_INET6)
        {
            char ip[INET6_ADDRSTRLEN] = {0,};
            struct sockaddr_in6* addr_v6 = (struct sockaddr_in6*)&rp->ai_addr;
            ::inet_ntop(AF_INET6, &(addr_v6->sin6_addr), ip, sizeof(ip));

            peeraddr->SetAddr(ip);
            peeraddr->SetPort(ntohs(addr_v6->sin6_port));
            peeraddr->SetIsIPv6(true);
        }

        list.push_back(peeraddr);
    }
}
