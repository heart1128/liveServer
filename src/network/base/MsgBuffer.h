#pragma once
#include <vector>
#include <string>
#include <algorithm>
#include <stdio.h>
#include <assert.h>
#include <cstring>

namespace tmms
{
    namespace network
    {
        
        static constexpr size_t kBufferDefaultLength{2048};
        static constexpr char CRLF[]{"\r\n"};

        /**
         * @brief This class represents a memory buffer used for sending and receiving
         * data.
         *
         */
        class MsgBuffer
        {
        public:
            /**
             * @brief Construct a new message buffer instance.
             *
             * @param len The initial size of the buffer.
             */
            MsgBuffer(size_t len = kBufferDefaultLength);

            /**
             * @brief Get the beginning of the buffer.
             *
             * @return const char*
             */
            const char *Peek() const
            {
                return begin() + head_;
            }

            /**
             * @brief Get the end of the buffer where new data can be written.
             *
             * @return const char*
             */
            const char *BeginWrite() const
            {
                return begin() + tail_;
            }
            char *BeginWrite()
            {
                return begin() + tail_;
            }

            /**
             * @brief Get a byte value from the buffer.
             *
             * @return uint8_t
             */
            uint8_t PeekInt8() const
            {
                assert(ReadableBytes() >= 1);
                return *(static_cast<const uint8_t *>((void *)Peek()));
            }

            /**
             * @brief Get a unsigned short value from the buffer.
             *
             * @return uint16_t
             */
            uint16_t PeekInt16() const;

            /**
             * @brief Get a unsigned int value from the buffer.
             *
             * @return uint32_t
             */
            uint32_t PeekInt32() const;

            /**
             * @brief Get a unsigned int64 value from the buffer.
             *
             * @return uint64_t
             */
            uint64_t PeekInt64() const;

            /**
             * @brief Get and remove some bytes from the buffer.
             *
             * @param len
             * @return std::string
             */
            std::string Read(size_t len);

            /**
             * @brief Get the remove a byte value from the buffer.
             *
             * @return uint8_t
             */
            uint8_t ReadInt8();

            /**
             * @brief Get and remove a unsigned short value from the buffer.
             *
             * @return uint16_t
             */
            uint16_t ReadInt16();

            /**
             * @brief Get and remove a unsigned int value from the buffer.
             *
             * @return uint32_t
             */
            uint32_t ReadInt32();

            /**
             * @brief Get and remove a unsigned int64 value from the buffer.
             *
             * @return uint64_t
             */
            uint64_t ReadInt64();

            /**
             * @brief swap the buffer with another.
             *
             * @param buf
             */
            void Swap(MsgBuffer &buf) noexcept;

            /**
             * @brief Return the size of the data in the buffer.
             *
             * @return size_t
             */
            size_t ReadableBytes() const
            {
                return tail_ - head_;
            }

            /**
             * @brief Return the size of the empty part in the buffer
             *
             * @return size_t
             */
            size_t WritableBytes() const
            {
                return buffer_.size() - tail_;
            }

            /**
             * @brief Append new data to the buffer.
             *
             */
            void Append(const MsgBuffer &buf);
            template <int N>
            void Append(const char (&buf)[N])
            {
                assert(strnlen(buf, N) == N - 1);
                Append(buf, N - 1);
            }
            void Append(const char *buf, size_t len);
            void Append(const std::string &buf)
            {
                Append(buf.c_str(), buf.length());
            }

            /**
             * @brief Append a byte value to the end of the buffer.
             *
             * @param b
             */
            void AppendInt8(const uint8_t b)
            {
                Append(static_cast<const char *>((void *)&b), 1);
            }

            /**
             * @brief Append a unsigned short value to the end of the buffer.
             *
             * @param s
             */
            void AppendInt16(const uint16_t s);

            /**
             * @brief Append a unsigned int value to the end of the buffer.
             *
             * @param i
             */
            void AppendInt32(const uint32_t i);

            /**
             * @brief Appaend a unsigned int64 value to the end of the buffer.
             *
             * @param l
             */
            void AppendInt64(const uint64_t l);

            /**
             * @brief Put new data to the beginning of the buffer.
             *
             * @param buf
             * @param len
             */
            void AddInFront(const char *buf, size_t len);

            /**
             * @brief Put a byte value to the beginning of the buffer.
             *
             * @param b
             */
            void AddInFrontInt8(const uint8_t b)
            {
                AddInFront(static_cast<const char *>((void *)&b), 1);
            }

            /**
             * @brief Put a unsigned short value to the beginning of the buffer.
             *
             * @param s
             */
            void AddInFrontInt16(const uint16_t s);

            /**
             * @brief Put a unsigned int value to the beginning of the buffer.
             *
             * @param i
             */
            void AddInFrontInt32(const uint32_t i);

            /**
             * @brief Put a unsigned int64 value to the beginning of the buffer.
             *
             * @param l
             */
            void AddInFrontInt64(const uint64_t l);

            /**
             * @brief Remove all data in the buffer.
             *
             */
            void RetrieveAll();

            /**
             * @brief Remove some bytes in the buffer.
             *
             * @param len
             */
            void Retrieve(size_t len);

            /**
             * @brief Read data from a file descriptor and put it into the buffer.˝
             *
             * @param fd The file descriptor. It is usually a socket.
             * @param retErrno The error code when reading.
             * @return ssize_t The number of bytes read from the file descriptor. -1 is
             * returned when an error occurs.
             */
            ssize_t ReadFd(int fd, int *retErrno);

            /**
             * @brief Remove the data before a certain position from the buffer.
             *
             * @param end The position.
             */
            void RetrieveUntil(const char *end)
            {
                assert(Peek() <= end);
                assert(end <= BeginWrite());
                Retrieve(end - Peek());
            }

            /**
             * @brief Find the position of the buffer where the CRLF is found.
             *
             * @return const char*
             */
            const char *FindCRLF() const
            {
                const char *crlf = std::search(Peek(), BeginWrite(), CRLF, CRLF + 2);
                return crlf == BeginWrite() ? NULL : crlf;
            }

            /**
             * @brief Make sure the buffer has enough spaces to write data.
             *
             * @param len
             */
            void EnsureWritableBytes(size_t len);

            /**
             * @brief Move the write pointer forward when the new data has been written
             * to the buffer.
             *
             * @param len
             */
            void HasWritten(size_t len)
            {
                assert(len <= WritableBytes());
                tail_ += len;
            }

            /**
             * @brief Move the write pointer backward to remove data in the end of the
             * buffer.
             *
             * @param offset
             */
            void Unwrite(size_t offset)
            {
                assert(ReadableBytes() >= offset);
                tail_ -= offset;
            }

            /**
             * @brief Access a byte in the buffer.
             *
             * @param offset
             * @return const char&
             */
            const char &operator[](size_t offset) const
            {
                assert(ReadableBytes() >= offset);
                return Peek()[offset];
            }
            char &operator[](size_t offset)
            {
                assert(ReadableBytes() >= offset);
                return begin()[head_ + offset];
            }

        private:
            size_t head_;
            size_t initCap_;
            std::vector<char> buffer_;
            size_t tail_;
            const char *begin() const
            {
                return &buffer_[0];
            }
            char *begin()
            {
                return &buffer_[0];
            }
        };

        inline void swap(MsgBuffer &one, MsgBuffer &two) noexcept
        {
            one.Swap(two);
        }
        inline uint64_t hton64(uint64_t n)
        {
            static const int one = 1;
            static const char sig = *(char *)&one;
            if (sig == 0)
                return n;  // for big endian machine just return the input
            char *ptr = reinterpret_cast<char *>(&n);
            std::reverse(ptr, ptr + sizeof(uint64_t));
            return n;
        }
        inline uint64_t ntoh64(uint64_t n)
        {
            return hton64(n);
        }        
    }  // namespace network


}
namespace std
{
    template <>
    inline void swap(tmms::network::MsgBuffer &one, tmms::network::MsgBuffer &two) noexcept
    {
        one.Swap(two);
    } 
}