/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-05 14:01:46
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-09 11:14:14
 * @FilePath: /liveServer/src/mmedia/base/AVTypes.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

#include <cstddef>
namespace tmms
{
    namespace mm
    {
        // flvheader + flvbody [previousTagSize + FlvTag[tagHeader+tagBody]]
        
        /**
         *  AudioTagHeader
         *     + SoundFormat	
         *     + SoundRate
         *      +
        */

        /// FLV音频编码帧的ID (AudioTagHeader)
        enum AudioCodecID {
            // 线性脉冲编码调制(Linear Pulse Code Modulation, LPCM)平台字节序编码
            kAudioCodecIDLinearPCMPlatformEndian = 0,
            // 自适应差分脉冲编码调制，一种压缩音频数据的方法
            kAudioCodecIDADPCM = 1,
            // MPEG-1 Audio Layer 3，一种广泛使用的音频压缩格式
            kAudioCodecIDMP3 = 2,
            // 线性PCM小端字节序编码
            kAudioCodecIDLinearPCMLittleEndian = 3,
            // Nellymoser编解码器，16kHz采样率的单声道音频
            kAudioCodecIDNellymoser16kHzMono = 4,
            // Nellymoser编解码器，8kHz采样率的单声道音频
            kAudioCodecIDNellymoser8kHzMono = 5,
            // Nellymoser编解码器，通常用于低码率音频传输
            kAudioCodecIDNellymoser = 6,
            // 保留的G.711 A-law对数PCM编码
            kAudioCodecIDReservedG711AlawLogarithmicPCM = 7,
            // 保留的G.711 μ-law对数PCM编码
            kAudioCodecIDReservedG711MuLawLogarithmicPCM = 8,
            // 保留的编解码器标识符
            kAudioCodecIDReserved = 9,
            // 高效率先进音频编码(Advanced Audio Coding, AAC)
            kAudioCodecIDAAC = 10,
            // Speex编解码器，一种开源的语音编解码器，适合低码率的语音传输
            kAudioCodecIDSpeex = 11,
            // Opus编解码器，一种开放标准的音频编解码器，适合语音和网络音频流
            kAudioCodecIDOpus = 13,
            // 保留的8kHz采样率的MP3编解码器标识符
            kAudioCodecIDReservedMP3_8kHz = 14,
            // 保留的设备特定声音编解码器标识符
            kAudioCodecIDReservedDeviceSpecificSound = 15,
        };


        /// @brief 音频采样率
        enum SoundRate
        {
            kSoundRate5512 = 0,
            kSoundRate11025 = 1,
            kSoundRate22050 = 2,
            kSoundRate44100 = 3,
            kSoundRate48000 = 4,
            kSoundRateNB8kHz   = 8,  // NB (narrowband)
            kSoundRateMB12kHz  = 12, // MB (medium-band)
            kSoundRateWB16kHz  = 16, // WB (wideband)
            kSoundRateSWB24kHz = 24, // SWB (super-wideband)
            kSoundRateFB48kHz  = 48, // FB (fullband)
            kSoundRateForbidden = 0xff,        
        };

        /// @brief 采样位深，大小
        enum SoundSize
        {        
            kSoundSizeBits8bit = 0,
            kSoundSizeBits16bit = 1,
            kSoundSizeBitsForbidden = 2,        
        };

        // flv定义的音频通道
        enum SoundChannel
        {
            kSoundChannelMono = 0,
            kSoundChannelStereo = 1,        // 立体双通道
            kSoundChannelForbidden = 2,        
        }; 

        // FLV定义AAC PakcetType
        enum AACPacketType
        {
            kAACPacketTypeAACSequenceHeader = 0,
            kAACPacketTypeAACRaw = 1,
        };

        // AAC SequenceHeader中的ObjectType
        enum AACObjectType
        {
            kAACObjectTypeForbidden = 0,
            kAACObjectTypeAacMain = 1,
            kAACObjectTypeAacLC = 2,
            kAACObjectTypeAacSSR = 3,
            kAACObjectTypeAacHE = 5,
            kAACObjectTypeAacHEV2 = 29,
        };

        // FLV定义的视频编码器ID
        enum VideoCodecID
        {
            kVideoCodecIDReserved = 0,
            kVideoCodecIDForbidden = 0,
            kVideoCodecIDReserved1 = 1,        
            kVideoCodecIDSorensonH263 = 2,
            kVideoCodecIDScreenVideo = 3,
            kVideoCodecIDOn2VP6 = 4,
            kVideoCodecIDOn2VP6WithAlphaChannel = 5,
            kVideoCodecIDScreenVideoVersion2 = 6,
            kVideoCodecIDAVC = 7,
            kVideoCodecIDDisabled = 8,
            kVideoCodecIDReserved2 = 9,
            kVideoCodecIDHEVC = 12,
            kVideoCodecIDAV1 = 13,
        };

        // FLV定义的视频帧类型
        enum FrameType
        {
            kFrameTypeKeyFrame = 1,
            kFrameTypeInterFrame = 2,
            kFrameTypeDisposableInterFrame = 3,
            kFrameTypeGeneratedKeyFrame = 4,
            kFrameTypeVideoInfoFrame = 5,
        }; 

        // FLV定义的AVCPacketType(h.264)
        enum AVCPacketType
        {
            kAVCPacketTypeForbidden = 3,
            
            kAVCPacketTypeSequenceHeader = 0,
            kAVCPacketTypeNALU = 1,
            kAVCPacketTypeSequenceHeaderEOF = 2,
        };

        // 9.AVC 视频帧类型
        enum NaluType
        {
            kNaluTypeForbidden = 0,

            kNaluTypeNonIDR = 1,
            kNaluTypeDataPartitionA = 2,
            kNaluTypeDataPartitionB = 3,
            kNaluTypeDataPartitionC = 4,
            kNaluTypeIDR = 5,
            kNaluTypeSEI = 6,
            kNaluTypeSPS = 7,
            kNaluTypePPS = 8,
            kNaluTypeAccessUnitDelimiter = 9,
            kNaluTypeEOSequence = 10,
            kNaluTypeEOStream = 11,
            kNaluTypeFilterData = 12,
            kNaluTypeSPSExt = 13,
            kNaluTypePrefixNALU = 14,
            kNaluTypeSubsetSPS = 15,
            kNaluTypeLayerWithoutPartition = 19,
            kNaluTypeCodedSliceExt = 20,
        }; 

        struct SampleBuf
        {
            SampleBuf(const char *buf, size_t s)
            :addr(buf), size(s)
            {}

            const char *addr{nullptr};
            size_t size{0};
        };
        

    } // namespace mm
    
} // namespace name
