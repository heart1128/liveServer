/*
 * @Author: heart1128 1020273485@qq.com
 * @Date: 2024-07-05 14:01:46
 * @LastEditors: heart1128 1020273485@qq.com
 * @LastEditTime: 2024-07-05 14:06:32
 * @FilePath: /liveServer/src/mmedia/base/AVTypes.h
 * @Description: 这是默认设置,请设置`customMade`, 打开koroFileHeader查看配置 进行设置: https://github.com/OBKoro1/koro1FileHeader/wiki/%E9%85%8D%E7%BD%AE
 */
#pragma once

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
        enum AudioCodecID
        {
            kAudioCodecIDLinearPCMPlatformEndian = 0,
            kAudioCodecIDADPCM = 1,
            kAudioCodecIDMP3 = 2,
            kAudioCodecIDLinearPCMLittleEndian = 3,
            kAudioCodecIDNellymoser16kHzMono = 4,
            kAudioCodecIDNellymoser8kHzMono = 5,
            kAudioCodecIDNellymoser = 6,
            kAudioCodecIDReservedG711AlawLogarithmicPCM = 7,
            kAudioCodecIDReservedG711MuLawLogarithmicPCM = 8,
            kAudioCodecIDReserved = 9,
            kAudioCodecIDAAC = 10,
            kAudioCodecIDSpeex = 11,
            kAudioCodecIDOpus = 13,
            kAudioCodecIDReservedMP3_8kHz = 14,
            kAudioCodecIDReservedDeviceSpecificSound = 15,
        };

        /// @brief 
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
    } // namespace mm
    
} // namespace name
