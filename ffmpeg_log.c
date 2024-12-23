#include <stdio.h>
#include <libavutil/log.h>

int main(int argc, char const *argv[])
{
    
    av_log_set_level(AV_LOG_ERROR); // set log level to error

    av_log(NULL, AV_LOG_ERROR, "This is an error message\n"); // log an error message
    
    return 0;
}
