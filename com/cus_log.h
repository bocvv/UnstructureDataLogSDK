//
// Created by xyhs on 6/1/18.
// Modifed by wugx on 9/4/21.
//

#ifndef CUS_LOG_H
#define CUS_LOG_H

//#define OPEN_LOG_PRINT

#ifndef OPEN_LOG_PRINT

#define LOGD(...) { }
#define LOGE(...) { }

#else

#ifdef __ANDROID__

#include <android/log.h>

#define LOG_TAG "aicv"
#define LOGD(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#elif _WIN32

#define LOGD(...) { }
#define LOGE(...) { }

#else
#include <sys/time.h>
#include <cstdio>

template <class T>
void print_time(FILE *fid) {
    struct timeval time;
    gettimeofday(&time, nullptr);
    T hourSec = time.tv_sec % (3600 * 24);
    T minSec = hourSec % 3600;
    T sec = minSec % 60;
    T hour = hourSec / 3600;
    T min = minSec / 60;
    fprintf(fid, "[%02ld:%02ld:%02ld %06ld] ", hour, min, sec, time.tv_usec);
}

#define LOGD(...) { print_time<long>(stdout); printf(__VA_ARGS__); printf("\n"); }
#define LOGE(...) { print_time<long>(stderr); fprintf(stderr, __VA_ARGS__); fprintf(stderr, "\n"); }

#endif

#endif //OPEN_LOG_PRINT

#endif //CUS_LOG_H
