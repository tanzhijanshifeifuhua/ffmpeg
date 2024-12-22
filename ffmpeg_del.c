#include <stdio.h>
#include <libavformat/avformat.h>
#include <libavutil/log.h>

int main(int argc, char **argv)
{
    int ret;
    av_log_set_level(AV_LOG_INFO);



    // ret = avpriv_io_delete("./test.txt");
    if (ret < 0) 
    {
        av_log(NULL, AV_LOG_ERROR, "Failed to delete file: %s\n", "./test.txt");
        return -1;
    }

    return 0;
}
