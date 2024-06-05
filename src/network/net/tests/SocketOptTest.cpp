/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-05 15:03:48
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-05 16:11:14
 * @FilePath: /tmms/src/network/net/tests/SocketOptTest.cpp
 * @Description:  learn 
 */
#include "network/base/InetAddress.h"
#include "network/base/SocketOpt.h"
#include <iostream>

using namespace tmms::base;

/// 客户端
// nc -l 34444模拟监听
void TestClient()
{
    // 创建一个socket
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    if(sock < 0)
    {
        std::cerr << "socket failed.sock" << sock << " errno:" << errno << std::endl;
        return;
    }

    // 服务器地址
    InetAddress server("0.0.0.0:34444");
    SocketOpt opt(sock);
    opt.SetNonBlocking(false);
    auto ret = opt.Connect(server);

    std::cout << "connect ret : " << ret << "   errno :" << errno << std::endl
                << "local: " << opt.GetLocalAddr()->ToIpPort() << std::endl
                << "peer: " << opt.GetPeerAddr()->ToIpPort() << std::endl;
}

// 模拟客户端
// ab -c 1 -n 1 "http://10.211.55.7:34444/"
void TestServer()
{
        // 创建一个socket
    int sock = SocketOpt::CreateNonblockingTcpSocket(AF_INET);
    if(sock < 0)
    {
        std::cerr << "socket failed.sock" << sock << " errno:" << errno << std::endl;
        return;
    }

    // 服务器地址
    InetAddress server("0.0.0.0:34444");
    SocketOpt opt(sock);
    opt.SetNonBlocking(false);

    opt.BindAddress(server);
    opt.Listen();
    InetAddress addr;
    auto ns = opt.Accept(&addr);

    std::cout << "accept ret : " << ns << "   errno :" << errno << std::endl
                << "addr: " << addr.ToIpPort() << std::endl;
}

int main()
{
    TestServer();
    return 0;
}