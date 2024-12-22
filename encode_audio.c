#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>

static int select_best_sample_rate(const AVCodec *pCodec)
{
    const int *pRate;
    int best_rate = 0;
    
    if(!pCodec->supported_samplerates)
    {
        return 44100;
    }
    pRate = pCodec->supported_samplerates;

    while (*pRate) {
        if (*pRate || abs(44100 - *pRate) < abs(best_rate - 44100)) {
            best_rate = *pRate;
            break;
        }
        pRate++;
    }
    return best_rate;
}

static int check_sample_fmt(const AVCodec *pCodec, enum AVSampleFormat sample_fmt)
{
    const enum AVSampleFormat *pSampleFormat = pCodec->sample_fmts;

    while (*pSampleFormat!= AV_SAMPLE_FMT_NONE) {
        if (*pSampleFormat == sample_fmt) {
            return 1;
        }
        pSampleFormat++;
    }
    return 0;
}

static int encode(AVCodecContext *pCodecCtx, AVFrame *pFrame, AVPacket *pPacket,FILE *pFile)
{
    int ret = -1;
    ret = avcodec_send_frame(pCodecCtx, pFrame);
    if(ret < 0)
    {
        av_log(pCodecCtx, AV_LOG_ERROR, "Error sending a frame for encoding\n");
        goto _END;
    }
    while (ret >= 0)
    {
        ret = avcodec_receive_packet(pCodecCtx, pPacket);
        if(ret == AVERROR(EAGAIN) || ret == AVERROR_EOF)
        {
            return 0;
        }
        else if(ret < 0)
        {
            av_log(pCodecCtx, AV_LOG_ERROR, "Error encoding a frame\n");
            return -1;
        }
        //写入文件
        fwrite(pPacket->data, 1, pPacket->size, pFile);
        av_packet_unref(pPacket);
    }
_END:
    return 0;
}

int main(int argc, char const *argv[])
{
    const char *pDst = NULL, *pCodecName = NULL;

    int ret = -1;
    FILE *pFile = NULL;
    uint16_t *sample = NULL;

    const AVCodec *pCodec = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    //1.输入参数进行判断
    if(argc < 2)
    {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 3\n");
        goto _ERROR;
    }
    pDst = argv[1];
    // pCodecName = argv[2];
    
    //2.查找编码器
    // pCodec = avcodec_find_encoder_by_name(pCodecName);
    pCodec = avcodec_find_encoder_by_name("libfdk_aac");
    // pCodec = avcodec_find_encoder(AV_CODEC_ID_AAC);
    if(!pCodec)
    {
        av_log(NULL, AV_LOG_ERROR, "codec not found%s\n",pCodecName);
        goto _ERROR;
    }
    //3.创建编码器上下文
    pCodecCtx = avcodec_alloc_context3(pCodec);
    if(!pCodecCtx)
    {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate codec context\n");
        goto _ERROR;
    }
    
    //4.设置编码器参数
    pCodecCtx->bit_rate = 64000;
    pCodecCtx->sample_fmt = AV_SAMPLE_FMT_S16;
    if(!check_sample_fmt(pCodec, pCodecCtx->sample_fmt))
    {
        av_log(pCodecCtx, AV_LOG_ERROR, "sample format not supported by codec\n");
        goto _ERROR;
    }
    pCodecCtx->sample_rate = select_best_sample_rate(pCodec);
    av_channel_layout_copy(&pCodecCtx->ch_layout, &(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);


    //5.编码器与编码器上下文绑定到一起
    ret = avcodec_open2(pCodecCtx, pCodec, NULL);
    if (ret < 0)
    {
        av_log(pCodecCtx, AV_LOG_ERROR, "failed to open codec:%s\n",av_err2str(ret));
        goto _ERROR;
    }
    
    //6.创建输出文件
    pFile = fopen(pDst,"wb");
    if(!pFile)
    {
        av_log(NULL, AV_LOG_ERROR, "failed to open output file:%s\n",pDst);
        goto _ERROR;
    }

    //7.创建AVFrame
    pFrame = av_frame_alloc();
    if(!pFrame)
    {   
        av_log(NULL, AV_LOG_ERROR, "failed to allocate frame\n");
        goto _ERROR;
    }

    pFrame->nb_samples = pCodecCtx->frame_size;
    pFrame->format = pCodecCtx->sample_fmt;
    av_channel_layout_copy(&pFrame->ch_layout,&(AVChannelLayout)AV_CHANNEL_LAYOUT_STEREO);

    ret = av_frame_get_buffer(pFrame, 0);
    if(ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate frame data\n");
        goto _ERROR;
    }
    //8.创建AVPacket
    pPacket = av_packet_alloc();
    if(!pPacket)
    {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate packet\n");
        goto _ERROR;
    }
    //9.生成音频内容
    float t = 0;
    float tincr = 2 * M_PI * 440 / pCodecCtx->sample_rate;
    for(int i = 0; i < 200; i++)
    {
        ret = av_frame_make_writable(pFrame);
        if(ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "failed to make frame writable\n");
            goto _ERROR;
        }
        sample = (uint16_t*)pFrame->data[0];
        for(int j = 0; j < pCodecCtx->frame_size; j++)
        {
            sample[2*j] = (int)(sin(t) * 10000);
            for(int k = 1; k < pCodecCtx->ch_layout.nb_channels; k++)
            {
                sample[2*j+k] = sample[2*j];
            }
            t = t + tincr;
        }
        encode(pCodecCtx, pFrame, pPacket, pFile);
    }
    //10.编码视频内容
    encode(pCodecCtx, NULL, pPacket, pFile);        //发送NULL帧，结束编码

_ERROR:
    if(pCodecCtx)
    {
        avcodec_free_context(&pCodecCtx);
        pCodecCtx = NULL;
    }

    if(pFrame)
    {
        av_frame_free(&pFrame);
        pFrame = NULL;
    }

    if (pPacket)
    {
        av_packet_free(&pPacket);
        pPacket = NULL;
    }
    
    return 0;
}
