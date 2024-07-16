/**
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-15 16:25:04
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-15 16:47:23
 * @FilePath: /liveServer/src/mmedia/mpegts/TestStreamWriter.cpp
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
**/
#include "TestStreamWriter.h"
#include "mmedia/base/MMediaLog.h"
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

using namespace tmms::mm;
TestStreamWriter::TestStreamWriter()
{
    fd_ = ::open("test.ts", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if(fd_ < 0)
    {
        MPEGTS_ERROR << "open test.ts failed.error:" << errno;
        return;
    }
}
TestStreamWriter::~TestStreamWriter()
{
    if(fd_ >= 0)
    {
        ::close(fd_);
        fd_ = -1;
    }
}
int32_t TestStreamWriter::Write(void *buf, uint32_t size)
{
    auto ret = ::write(fd_, buf, size);
    if(ret != size)
    {
        MPEGTS_WARN << "write ret:" << ret << " size:" << size;
    }
    return 0;
}