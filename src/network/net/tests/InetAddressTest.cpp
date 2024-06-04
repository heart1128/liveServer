/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-04 21:36:56
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-04 21:40:04
 * @FilePath: /tmms/src/network/net/tests/InetAddressTest.cpp
 * @Description:  learn 
 */
#include "network/base/InetAddress.h"
#include <string>
#include <iostream>

using namespace tmms::network;

int main()
{
    std::string host;
    while(std::cin >> host)
    {
        InetAdress addr(host);
        std::cout << "host:" << host << std::endl
                    << "ip:" << addr.IP() << std::endl
                    << "port:" << addr.Port() << std::endl
                    << "lan:" << addr.IsLanIp() << std::endl
                    << "wan:" << addr.IsWanIp() << std::endl
                    << "loop:" << addr.IsLoopbackIp() << std::endl;
    }
    return 0;
}