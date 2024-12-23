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

    AVFormatContext *pFormatContext = NULL;
    AVFormatContext *pOutputContext = NULL;
    const AVOutputFormat *pOutputFormat = NULL;
    AVStream *pOutputStream = NULL;
    AVStream *pInputStream = NULL;
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
        exit(-1);
    }
    // 3、获取视频流
    index = av_find_best_stream(pFormatContext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (index < 0)
    {
        av_log(pFormatContext, AV_LOG_ERROR, "Could not find audio stream in source file\n");
        goto _EEROR;
    }
    // 4、打开目的文件上下文
    pOutputContext = avformat_alloc_context();
    if (pOutputContext == NULL)
    {
        av_log(NULL, AV_LOG_ERROR, "Could not allocate output context\n");
        goto _EEROR;
    }
    pOutputFormat = av_guess_format(NULL, dst, NULL);

    pOutputContext->oformat = pOutputFormat;

    // 5、创建新的视频流
    pOutputStream = avformat_new_stream(pOutputContext, NULL);

    // 6、设置输出视频参数
    pInputStream = pFormatContext->streams[index];
    avcodec_parameters_copy(pOutputStream->codecpar, pInputStream->codecpar);
    pOutputStream->codecpar->codec_tag = 0; // 根据编码器类型自动设置tag

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
    // 8、循环读取视频流数据，写入目的文件
    while (av_read_frame(pFormatContext, &packet) >= 0)
    {
        if (packet.stream_index == index)
        {
            packet.pts = av_rescale_q_rnd(packet.pts, pInputStream->time_base, pOutputStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.dts = av_rescale_q_rnd(packet.dts, pInputStream->time_base, pOutputStream->time_base, (AV_ROUND_NEAR_INF | AV_ROUND_PASS_MINMAX));
            packet.duration = av_rescale_q(packet.duration, pInputStream->time_base, pOutputStream->time_base);
            packet.stream_index = 0;
            packet.pos = -1;

            av_interleaved_write_frame(pOutputContext, &packet);
            av_packet_unref(&packet);
        }
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

    return 0;
}