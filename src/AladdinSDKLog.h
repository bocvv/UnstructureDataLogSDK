#ifndef __ALADDINSDK_LOG_H__
#define __ALADDINSDK_LOG_H__


#include <stdio.h>
#include <stdlib.h>

#if !defined(ALADDINSDK_API)
    #if defined(_WIN32) || defined(__CYGWIN__)
        #if defined(BUILDING_SHARED_LIBRARY)
            #define ALADDINSDK_API __declspec(dllexport)
        #elif defined(USING_SHARED_LIBRARY)
            #define ALADDINSDK_API __declspec(dllimport)
        #else
            #define ALADDINSDK_API
        #endif // BUILDING_SHARED_LIBRARY
    #elif defined(__GNUC__) && (__GNUC__ >= 4) && !defined(__APPLE__)
        #define ALADDINSDK_API __attribute__((visibility("default")))
    #else
        #define LIBYUV_API
    #endif // __GNUC__
#endif // ALADDINSDK_API

// 引擎句柄
#define aladdinSDK_log_handle void*

// 日志类型标识便于后续检索
typedef enum aladdinSDK_log_type {
    ALADDINSDK_LOG_IMAGE,
    ALADDINSDK_LOG_VIDEO,
    ALADDINSDK_LOG_AUDIO,
    ALADDINSDK_LOG_TEXTS,
    ALADDINSDK_LOG_OTHER
};

// 日志发送进度获取回调函数
typedef bool (*aladdinSDK_log_cb)(int);


#if defined(__cplusplus)
extern "C" {
#endif

// 初始化函数
ALADDINSDK_API bool aladdinSDK_log_init(aladdinSDK_log_handle* plog_handle, const char* module_name, const char* module_version, const char* sdk_name, const char* sdk_version);

// 检查日志记录权限
ALADDINSDK_API bool aladdinSDK_log_auth(aladdinSDK_log_handle log_handle, int* plog_auth);

// 推送日志相关信息
ALADDINSDK_API bool aladdinSDK_log_push(aladdinSDK_log_handle log_handle, aladdinSDK_log_type log_type, const char* log_buf, int log_len, long log_time, const char* log_str, aladdinSDK_log_cb log_cb);

// 读取库版本字符串
ALADDINSDK_API bool aladdinSDK_log_vers(aladdinSDK_log_handle log_handle, char* plog_vers);

// 库释放函数
ALADDINSDK_API bool aladdinSDK_log_free(aladdinSDK_log_handle* plog_handle);

#if defined(__cplusplus)
}
#endif


#endif // __ALADDINSDK_LOG_H__

