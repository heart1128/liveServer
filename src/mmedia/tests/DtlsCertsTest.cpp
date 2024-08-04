/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-08-04 18:30:28
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-08-04 18:31:09
 * @FilePath: /liveServer/src/mmedia/tests/DtlsCertsTest.cpp
 * @Description:  learn 
 */
#include "mmedia/webrtc/DtlsCerts.h"
#include <iostream>

using namespace tmms::mm;

int main()
{
    DtlsCerts certs;
    certs.Init();
    std::cout << certs.Fingerprint() << std::endl;
    return 0;
}