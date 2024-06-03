/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-01 20:55:06
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-03 11:34:36
 * @FilePath: /tmms/src/base/FileLog.cpp
 * @Description:  learn
 */
#include "FileLog.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

using namespace tmms::base;

/// @brief 
/// @param filePath 
/// @return 
bool FileLog::Open(const std::string &filePath)
{
    file_path_ = filePath;
    // Append操作是原子操作，不需要先定位文件指针，然后写
    int fd = ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
    if(fd < 0)
    {
        std::cout << "open file log error.ptah = " << filePath << std::endl;
        return false;
    }

    fd_ = fd;
    return true;
}

/// @brief 
/// @param msg 
/// @return 
size_t FileLog::WriteLog(const std::string &msg)
{
    int fd = fd_ == -1 ? 1 : fd_;               // 如果是-1，没有打开文件就输出到标准输出1中
    return ::write(fd, msg.data(), msg.size());
}

/// @brief 打开新的文件描述符，替换原有的，之后都会写入到这个新的
/// @param file 
void FileLog::Rotate(const std::string &file)
{
    if(file_path_.empty())      // 没有打开过文件
    {
        return;
    }

    int ret = ::rename(file_path_.c_str(), file.c_str());
    if(ret != 0)
    {
        std::cerr << "rename failed. old : " << file_path_ << "new :"  << file;
        return;
    }
    int fd = ::open(file_path_.c_str(), O_CREAT | O_APPEND | O_WRONLY, DEFFILEMODE);
    if(fd < 0)
    {
        std::cout << "open file log error.ptah = " << file << std::endl;
        return;
    }

    // https://blog.csdn.net/wmzjzwlzs/article/details/131212336
    // 进行复制的时候和write是并发的，但是不会导致出错
    // 上面是用append打开的，当执行write的时候会锁定文件的inode
    // 在write没有完成之前，这个复制不能成功，等于上锁了
    ::dup2(fd, fd_);        // 复制一个文件描述符到fd_,dup2是可以指定复制的文件描述符值，dup是返回最小可用的
    close(fd);
}

/// @brief 
/// @param type 
void FileLog::SetRotate(RotateType type)
{
    rotate_type_ = type;
}

/// @brief 
/// @return 
RotateType FileLog::GetRotateType() const
{
    return rotate_type_;
}

/// @brief 
/// @return 
int64_t FileLog::FileSize() const
{
    // lseek64实现文件跳转，返回挑转的长度，这里直接从0跳转到end
    return ::lseek64(fd_, 0, SEEK_END);
}

/// @brief 
/// @return 
std::string FileLog::FilePath() const
{
    return file_path_;
}
