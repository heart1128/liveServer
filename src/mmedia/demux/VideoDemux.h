#pragma once

#include <cstddef>
#include <list>
#include <string>
#include <cstdint>
#include "mmedia/base/AVTypes.h"

namespace tmms
{
    namespace mm
    {
        // AVC（H.254）有两个格式，一个是AVCC，一个是AnnexB
        enum AVCPayloadFormat
        {
            kPayloadFormatUnkonwed = 0,
            kPayloadFormatAvcc = 1,
            kPayloadFormatAnnexB = 2,
        };

        class VideoDemux
        {
        public:
            VideoDemux() = default;
            ~VideoDemux() = default;

            /**
             * @description: 解析FLV的VIDEODATA
             *              FrameType 4 帧类型
             *              CodecID 4 视频编码
             *              IDVideoData N*8 视频数据
             * @param {char} *data
             * @param {size_t} size
             * @param {list<SampleBuf>} &
             * @return {*}
            **/            
            int32_t OnDemux(const char *data,size_t size,std::list<SampleBuf> & outs);
            bool HasIdr() const; 
            bool HasAud() const; 
            bool HasSpsPps() const;

            VideoCodecID GetCodecID() const
            {
                return codec_id_;
            }

            int32_t GetCST() const
            {
                return composition_time_;
            }
            const std::string &GetSPS() const
            {
                return sps_;
            }
            const std::string &GetPPS() const
            {
                return pps_;
            }
            void Reset()
            {
                has_aud_ = false;
                has_idr_ = false;
                has_pps_sps_ = false;
            }
        
        private:  
            int32_t DemuxAVC(const char *data,size_t size,std::list<SampleBuf> &outs);

            /**
             * @description: 找到annexb的NALU开始，第一帧是4字节的start code, 其他都是3字节的
             * @param {char} *p
             * @param {char} *end
             * @return {*}
            **/            
            const char* FindAnnexbNalu(const char *p, const char *end);

            /**
             * @description: 
             * @param {char} *data
             * @param {size_t} size
             * @param {list<SampleBuf>} &outs
             * @return {*}
            **/            
            int32_t DecodeAVCNaluAnnexb(const char *data,size_t size, std::list<SampleBuf> &outs);

            /**
             * @description: 
             * @param {char} *data
             * @param {size_t} size
             * @param {list<SampleBuf>} &outs
             * @return {*}
            **/            
            int32_t DecodeAVCNaluIAvcc(const char *data,size_t size, std::list<SampleBuf> &outs);

            /**
             * @description:    
                        * 名称	                比特数	描述
                        configurationVersion	8	版本号，总是1
                        AVCProfileIndication	8	sps[1]
                        profile_compatibility	8	sps[2]
                        AVCLevelIndication	8	sps[3]

                        configurationVersion,AVCProfileIndication,profile_compatibility,AVCLevelIndication：都是一个字节，具体的内容由解码器去理解。
                        lengthSizeMinusOne：unit_length长度所占的字节数减1，也即lengthSizeMinusOne的值+1才是unit_length所占用的字节数。
                        numOfSequenceParameterSets：sps的个数
                        sequenceParameterSetLength：sps内容的长度
                        sequenceParameterSetNALUnit：sps的内容
                        numOfPictureParameterSets：pps的个数
                        pictureParameterSetLength：pps内容的长度
                        pictureParameterSetNALUnit：pps的内容
                        当VideoData为AVC RAW时，AVC RAW的结构是avcc的。
             * @param {char} *data
             * @param {size_t} size
             * @param {list<SampleBuf>} &
             * @return {*}
            **/            
            int32_t DecodeAVCSeqHeader(const char *data,size_t size,std::list<SampleBuf> & outs);

            /**
             * @description: AVC码流有两种格式：
                        avcC格式也叫AVC1格式，MPEG-4格式，字节对齐，因此也叫Byte-Stream Format。用于mp4、flv、mkv等封装中。
                        AnnexB格式也叫MPEG-2 transport stream format格式（ts格式），ElementaryStream格式。用于ts流中。
             * @param {char} *data
             * @param {size_t} size
             * @param {list<SampleBuf>} &
             * @return {*}
            **/            
            int32_t DecodeAvcNalu(const char *data,size_t size,std::list<SampleBuf> & outs);

            /**
             * @description: 一个NALU = 一组对应于视频编码的NALU头部信息 + 一个原始字节序列负荷(RBSP,Raw Byte Sequence Payload).
             *          NALU header占用一个字节，第五位是type
             * @param {char} *data
             * @return {*}
            **/            
            void CheckNaluType(const char *data);  


        
        private:
        /**  AVC seqHeader **/
            VideoCodecID codec_id_;
            int32_t composition_time_{0}; // 修正时间戳
            uint8_t config_version_{0};
            uint8_t profile_{0};
            uint8_t profile_com_{0};
            uint8_t level_{0};
            uint8_t nalu_unit_length_{0}; //NALU占用的字节数

            bool avc_ok_{false};
            std::string sps_;   // 后续一系列的视频编码信息，编解码使用
            std::string pps_;   // 后续一个图像的编码信息
            AVCPayloadFormat payload_format_{kPayloadFormatUnkonwed};
            bool has_aud_{false};         // 分隔符
            bool has_idr_{false};
            bool has_pps_sps_{false};
        };
        
        
    } // namespace mm
    
} // namespace tmms
