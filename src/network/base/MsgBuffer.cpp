#include "MsgBuffer.h"
#include <string.h>
#include <sys/uio.h>
#include <netinet/in.h>
#include <errno.h>
#include <assert.h>

using namespace tmms::network;

namespace tmms::network
{
    static constexpr size_t kBufferOffset{8};
}

MsgBuffer::MsgBuffer(size_t len)
    : head_(kBufferOffset), initCap_(len), buffer_(len + head_), tail_(head_)
{
}

void MsgBuffer::EnsureWritableBytes(size_t len)
{
    if (WritableBytes() >= len)
        return;
    if (head_ + WritableBytes() >=
        (len + kBufferOffset))  // move Readable bytes
    {
        std::copy(begin() + head_, begin() + tail_, begin() + kBufferOffset);
        tail_ = kBufferOffset + (tail_ - head_);
        head_ = kBufferOffset;
        return;
    }
    // create new buffer
    size_t newLen;
    if ((buffer_.size() * 2) > (kBufferOffset + ReadableBytes() + len))
        newLen = buffer_.size() * 2;
    else
        newLen = kBufferOffset + ReadableBytes() + len;
    MsgBuffer newbuffer(newLen);
    newbuffer.Append(*this);
    Swap(newbuffer);
}
void MsgBuffer::Swap(MsgBuffer &buf) noexcept
{
    buffer_.swap(buf.buffer_);
    std::swap(head_, buf.head_);
    std::swap(tail_, buf.tail_);
    std::swap(initCap_, buf.initCap_);
}
void MsgBuffer::Append(const MsgBuffer &buf)
{
    EnsureWritableBytes(buf.ReadableBytes());
    memcpy(&buffer_[tail_], buf.Peek(), buf.ReadableBytes());
    tail_ += buf.ReadableBytes();
}
void MsgBuffer::Append(const char *buf, size_t len)
{
    EnsureWritableBytes(len);
    memcpy(&buffer_[tail_], buf, len);
    tail_ += len;
}
void MsgBuffer::AppendInt16(const uint16_t s)
{
    uint16_t ss = htons(s);
    Append(static_cast<const char *>((void *)&ss), 2);
}
void MsgBuffer::AppendInt32(const uint32_t i)
{
    uint32_t ii = htonl(i);
    Append(static_cast<const char *>((void *)&ii), 4);
}
void MsgBuffer::AppendInt64(const uint64_t l)
{
    uint64_t ll = hton64(l);
    Append(static_cast<const char *>((void *)&ll), 8);
}

void MsgBuffer::AddInFrontInt16(const uint16_t s)
{
    uint16_t ss = htons(s);
    AddInFront(static_cast<const char *>((void *)&ss), 2);
}
void MsgBuffer::AddInFrontInt32(const uint32_t i)
{
    uint32_t ii = htonl(i);
    AddInFront(static_cast<const char *>((void *)&ii), 4);
}
void MsgBuffer::AddInFrontInt64(const uint64_t l)
{
    uint64_t ll = hton64(l);
    AddInFront(static_cast<const char *>((void *)&ll), 8);
}

uint16_t MsgBuffer::PeekInt16() const
{
    assert(ReadableBytes() >= 2);
    uint16_t rs = *(static_cast<const uint16_t *>((void *)Peek()));
    return ntohs(rs);
}
uint32_t MsgBuffer::PeekInt32() const
{
    assert(ReadableBytes() >= 4);
    uint32_t rl = *(static_cast<const uint32_t *>((void *)Peek()));
    return ntohl(rl);
}
uint64_t MsgBuffer::PeekInt64() const
{
    assert(ReadableBytes() >= 8);
    uint64_t rll = *(static_cast<const uint64_t *>((void *)Peek()));
    return ntoh64(rll);
}

void MsgBuffer::Retrieve(size_t len)
{
    if (len >= ReadableBytes())
    {
        RetrieveAll();
        return;
    }
    head_ += len;
}
void MsgBuffer::RetrieveAll()
{
    if (buffer_.size() > (initCap_ * 2))
    {
        buffer_.resize(initCap_);
    }
    tail_ = head_ = kBufferOffset;
}
ssize_t MsgBuffer::ReadFd(int fd, int *retErrno)
{
    char extBuffer[8192];
    struct iovec vec[2];
    size_t writable = WritableBytes();
    vec[0].iov_base = begin() + tail_;
    vec[0].iov_len = static_cast<int>(writable);
    vec[1].iov_base = extBuffer;
    vec[1].iov_len = sizeof(extBuffer);
    const int iovcnt = (writable < sizeof extBuffer) ? 2 : 1;
    ssize_t n = ::readv(fd, vec, iovcnt);
    if (n < 0)
    {
        *retErrno = errno;
    }
    else if (static_cast<size_t>(n) <= writable)
    {
        tail_ += n;
    }
    else
    {
        tail_ = buffer_.size();
        Append(extBuffer, n - writable);
    }
    return n;
}

std::string MsgBuffer::Read(size_t len)
{
    if (len > ReadableBytes())
        len = ReadableBytes();
    std::string ret(Peek(), len);
    Retrieve(len);
    return ret;
}
uint8_t MsgBuffer::ReadInt8()
{
    uint8_t ret = PeekInt8();
    Retrieve(1);
    return ret;
}
uint16_t MsgBuffer::ReadInt16()
{
    uint16_t ret = PeekInt16();
    Retrieve(2);
    return ret;
}
uint32_t MsgBuffer::ReadInt32()
{
    uint32_t ret = PeekInt32();
    Retrieve(4);
    return ret;
}
uint64_t MsgBuffer::ReadInt64()
{
    uint64_t ret = PeekInt64();
    Retrieve(8);
    return ret;
}

void MsgBuffer::AddInFront(const char *buf, size_t len)
{
    if (head_ >= len)
    {
        memcpy(begin() + head_ - len, buf, len);
        head_ -= len;
        return;
    }
    if (len <= WritableBytes())
    {
        std::copy(begin() + head_, begin() + tail_, begin() + head_ + len);
        memcpy(begin() + head_, buf, len);
        tail_ += len;
        return;
    }
    size_t newLen;
    if (len + ReadableBytes() < initCap_)
        newLen = initCap_;
    else
        newLen = len + ReadableBytes();
    MsgBuffer newBuf(newLen);
    newBuf.Append(buf, len);
    newBuf.Append(*this);
    Swap(newBuf);
}
