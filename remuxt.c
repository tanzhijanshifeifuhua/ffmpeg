#include <stdio.h>
#include <libavutil/log.h>
#include <libavutil/avutil.h>
#include <libavformat/avformat.h>

int main(int argc, char *argv[])
{

    // 1、处理参数
    char *src;
    char *dst;
    int ret;
    int index;
    int i = 0;

    int streamIndex = 0;
    int *pstreamMap = NULL;

    AVFormatContext *pFormatContext = NULL;
    AVFormatContext *pOutputContext = NULL;
    const AVOutputFormat *pOutputFormat = NULL;
    AVPacket packet;

    av_log_set_level(AV_LOG_DEBUG);
    if (argc < 3)
    {
        av_log(NULL, AV_LOG_INFO, "arguments must be more than 3\n");
        exit(-1);
    }

    src = argv[1];
    dst = argv[2];
    // 2、打开多媒体文件
    ret = avformat_open_input(&pFormatContext, src, NULL, NULL);
    if (ret < 0)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not open source file%s\n", av_err2str(ret));
        goto _EEROR;
    }
    // 4、打开目的文件上下文
    avformat_alloc_output_context2(&pOutputContext, NULL, NULL, dst);
    if (pOutputContext == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not create output context\n");
        goto _EEROR;
    }
    // 5、创建新的视频流
    pstreamMap = av_calloc(pFormatContext->nb_chapters, sizeof(int));
    if (pstreamMap == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate stream map\n");
        goto _EEROR;
    }
    for(i = 0; i < pFormatContext->nb_streams; i++)
    {
        AVStream *pOutputStream = NULL;
        AVStream *pInputStream = NULL;
        pInputStream = pFormatContext->streams[i];
        AVCodecParameters *pCodecPar = pInputStream->codecpar;
        if (pCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
            pCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
            pCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE)
        {
            pstreamMap[i] = -1;
        }
        pstreamMap[i] = streamIndex++;
        pOutputStream = avformat_new_stream(pOutputContext, NULL);
        if (pOutputStream == NULL)
        {
            av_log(pOutputContext, AV_LOG_ERROR, "Could not create output stream\n");
            goto _EEROR;
        }

        avcodec_parameters_copy(pOutputStream->codecpar, pInputStream->codecpar);
        pOutputStream->codecpar->codec_tag = 0; // 根据编码器类型自动设置tag
    }


    // 绑定
    ret = avio_open2(&pOutputContext->pb, dst, AVIO_FLAG_WRITE, NULL, NULL);
    if (ret < 0)
    {
        av_log(pOutputContext, AV_LOG_ERROR, "Could not open output file%s\n", av_err2str(ret));
        goto _EEROR;
    }
    // 7、写多媒体文件头到目的文件
    ret = avformat_write_header(pOutputContext, NULL);
    if (ret < 0)
    {
        av_log(pOutputContext, AV_LOG_ERROR, "Error occurred when writing header%s\n", av_err2str(ret));
        goto _EEROR;
    }
    // 8、循环读取音视频字幕流数据，写入目的文件
    while (av_read_frame(pFormatContext, &packet) >= 0)
    {
        AVStream *pOutputStream = NULL;
        AVStream *pInputStream = NULL;

        pInputStream = pFormatContext->streams[packet.stream_index];
        if (pstreamMap[packet.stream_index] < 0)
        {
            av_packet_unref(&packet);
            continue;
        }
        packet.stream_index = pstreamMap[packet.stream_index];
        pOutputStream = pOutputContext->streams[packet.stream_index];

        av_packet_rescale_ts(&packet, pInputStream->time_base, pOutputStream->time_base);
        packet.pos = -1;
        av_interleaved_write_frame(pOutputContext, &packet);
        av_packet_unref(&packet);
    }
    // 9、写多媒体文件尾到目的文件
    av_write_trailer(pOutputContext);
    // 10、将申请资源进行释放
_EEROR:
    if (pFormatContext != NULL)
    {
        avformat_close_input(&pFormatContext);
        pFormatContext = NULL;
    }
    if (pOutputContext->pb != NULL)
    {
        avio_close(pOutputContext->pb);
    }
    if (pOutputContext != NULL)
    {
        avformat_free_context(pOutputContext);
        pOutputContext = NULL;
    }
    if (pstreamMap != NULL)
    {
        av_free(pstreamMap);
        pstreamMap = NULL;
    }

    return 0;
}