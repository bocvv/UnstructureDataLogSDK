#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <time.h>

#include "AladdinSDKLog.h"

#define MAX_FILE_SIZE  10*1024*1024 // 最大发送10MB数据

int percent = 0;

static long getCurrentTimeStamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static bool getLogPushProcess(int status)
{
    percent = status;
    printf("log push with %d%\n", percent);
    return percent >= 100;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        printf("command : ./AladdinUDPClient file_name cycle_count\n");
        return -1;
    }

    FILE* fp = fopen(argv[1], "r");
    if (NULL == fp) {
        printf("file not found %s\n", argv[1]);
        return -1;
    }
    fseek(fp, 0, SEEK_END);
    long lSize = ftell(fp);
    rewind(fp);

    int file_len = lSize > MAX_FILE_SIZE ? MAX_FILE_SIZE : (int)lSize;
    char* file_data = (char*)malloc(file_len * sizeof(char));
    if (NULL == file_data) {
        printf("malloc memory failed %d\n", lSize);
        return 1;
    }

    fread(file_data, file_len, 1, fp);

#if 1
    const char* module_name = "GoodQuestion";
    const char* module_version = "0.0.1";
    const char* sdk_name = "FileTransfer";
    const char* sdk_version = "0.0.1";

    bool ret = false;

    aladdinSDK_log_handle log_handle = NULL;
    ret = aladdinSDK_log_init(&log_handle, module_name, module_version, sdk_name, sdk_version);

    char log_vers[16] = {0};
    ret = aladdinSDK_log_vers(log_handle, log_vers);
    printf("aladdinSDK log_init succeed %s\n", log_vers);

    int cycle_count = (argc <= 2) ? INT_MAX : atoi(argv[2]);
    printf("aladdinSDK log_auth+push %d times\n", cycle_count);
    while (cycle_count > 0) {
        int log_auth = -1;
        ret = aladdinSDK_log_auth(log_handle, &log_auth);
        printf("log auth done %d\n", log_auth);

        if (log_auth < 0) {
            printf("log have not authorized, wait\n");
            sleep(1);
            continue; // 没有获取到配置，等下再试
        } 
        else if (log_auth == 0) {
            printf("log is not authorized, exit\n");
            break; // 获取到配置：不允许，直接退出
        }

        long timestamp = getCurrentTimeStamp();
        ret = aladdinSDK_log_push(log_handle, ALADDINSDK_LOG_IMAGE, file_data, file_len, timestamp, "debug", getLogPushProcess);
        while (percent < 100) {
           usleep(1000);
        }
        printf("log push done %d bytes with %ld ms\n", file_len, getCurrentTimeStamp() - timestamp);

        cycle_count --;
        printf("log push leaves %d times\n", cycle_count);
    }

    ret = aladdinSDK_log_free(&log_handle);
    printf("aladdinSDK log_free succeed %s\n", log_vers);
#endif

    if (file_data) {
        free(file_data);
        file_data = NULL;
    }

    if (fp) {
       fclose(fp);
       fp = NULL;
    }

    return 0;
}

