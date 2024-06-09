/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-06-09 16:40:42
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-06-09 16:57:27
 * @FilePath: /tmms/src/mmedia/base/Packet.cpp
 * @Description:  learn 
 */
#include "Packet.h"

using namespace tmms::mm;

Packet::ptr Packet::NewPacket(int32_t size)
{
    // 类本身也占空间
    auto block_size = size + sizeof(Packet);
    Packet* packet = (Packet*)new char[block_size];
    memset((void*)packet, 0x00, block_size);
    packet->index_ = -1;
    packet->type_ = kPacketTypeUnKnowed;
    packet->capacity_ = size;
    packet->ext_.reset();

    // 返回带自定义删除器的智能指针
    return Packet::ptr(packet, [](Packet *p){
        delete [] (char*)p;
    });
}
