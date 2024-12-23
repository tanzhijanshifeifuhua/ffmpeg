#include <libavutil/log.h>
#include <libavformat/avformat.h>

int main(int argc, char const *argv[])
{
    int ret;
    av_log_set_level(AV_LOG_INFO);
    // ret = avpriv_io_delete("./test.txt");
    if (ret < 0) {
        av_log(NULL, AV_LOG_ERROR, " detected ./test.txt error\n");
        return -1;
        }else{
            av_log(NULL, AV_LOG_INFO, " delete ./test.txt success\n");
        }

    return 0;
}

