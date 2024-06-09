#include "network/DnsServer.h"
#include <iostream>

using namespace tmms::network;

int main(int argc, char* argv[])
{
    std::vector<InetAddress::ptr> list;
    sDnsServer->AddHost("www.baidu.com");
    sDnsServer->Start();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    list = sDnsServer->GetHostAddress("www.baidu.com");
    for(auto &i : list)
    {
        std::cout << "ip: " << i->IP() << std::endl;
    }
    return 0;
}