#include <stdio.h>
#include <libavcodec/avcodec.h>
#include <libavutil/log.h>
#include <libavutil/opt.h>
#include <libavformat/avformat.h>

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

    const AVCodec *pCodec = NULL;
    AVCodecContext *pCodecCtx = NULL;
    AVFrame *pFrame = NULL;
    AVPacket *pPacket = NULL;

    av_log_set_level(AV_LOG_DEBUG);
    //1.输入参数进行判断
    if(argc < 3)
    {
        av_log(NULL, AV_LOG_ERROR, "arguments must be more than 3\n");
        goto _ERROR;
    }
    pDst = argv[1];
    pCodecName = argv[2];
    
    //2.查找编码器
    pCodec = avcodec_find_encoder_by_name(pCodecName);
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
    pCodecCtx->width = 640;
    pCodecCtx->height = 480;
    pCodecCtx->bit_rate = 500000;  //kpbs

    pCodecCtx->time_base = (AVRational){1, 25};     //时间基
    pCodecCtx->framerate = (AVRational){25, 1};     //帧率

    pCodecCtx->gop_size = 10;
    pCodecCtx->max_b_frames = 1;       //最大B帧数
    pCodecCtx->pix_fmt = AV_PIX_FMT_YUV420P;

    if(pCodec->id == AV_CODEC_ID_H264)
    {
        av_opt_set(pCodecCtx->priv_data, "preset", "slow",0);
    }
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

    pFrame->width = pCodecCtx->width;
    pFrame->height = pCodecCtx->height;
    pFrame->format = pCodecCtx->pix_fmt;

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
    //9.生成视频内容
    for(int i = 0; i < 25; i++)
    {
        ret = av_frame_make_writable(pFrame);
        if(ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "failed to make frame writable\n");
            goto _ERROR;
        }
        //Y分量，亮度
        for(int y = 0; y < pFrame->height; y++)
        {
            for(int x = 0; x < pFrame->width; x++)
            {
                pFrame->data[0][y * pFrame->linesize[0] + x] = x + y + i * 3;        
            }
        }
        //UV分量,颜色
        for(int y = 0; y < pFrame->height/2; y++)
        {
            for(int x = 0; x < pFrame->width/2; x++)
            {
                pFrame->data[1][y * pFrame->linesize[1] + x] = 128 + x + y + i * 2;  //128黑色      
                pFrame->data[2][y * pFrame->linesize[2] + x] = 64 + x + y + i * 2;   //64黑色      
            }
        }

        pFrame->pts = i;
        //10.编码一帧视频
        ret = encode(pCodecCtx, pFrame, pPacket, pFile);
        if(ret == -1)
        {
            av_log(NULL, AV_LOG_ERROR, "failed to encode frame\n");
            goto _ERROR;
        }
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
