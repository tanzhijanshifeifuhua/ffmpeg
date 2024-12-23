#include <stdio.h>
#include <libavutil/log.h>
#include <libavformat/avformat.h>

int main(int argc, char **argv)
{
    AVIODirContext *avio_dir_context = NULL;
    AVIODirEntry *tag = NULL;
    int ret;
    av_log_set_level(AV_LOG_INFO);

    ret = avio_open_dir(&avio_dir_context, "./", NULL);
    if (ret < 0) 
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to open directory: %s\n", av_err2str(ret));
        return -1;
    }
    while (1) 
    {
        ret = avio_read_dir(avio_dir_context, &tag);
        if (ret < 0)
        {
            av_log(NULL, AV_LOG_ERROR, "Failed to open directory: %s\n", av_err2str(ret));
            goto __fail;
        }
        if (!tag)
        {
            break;
        }
        av_log(NULL, AV_LOG_INFO, "%12" PRId64 " %s\n", tag->size, tag->name);
        avio_free_directory_entry(&tag);
    }
__fail:
    avio_close_dir(&avio_dir_context);
    return 0;
}