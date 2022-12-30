#ifndef __ALADDINSDK_LOG_PACK_H__
#define __ALADDINSDK_LOG_PACK_H__

#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <chrono>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>

#include "AladdinSDKLogComm.h"
#include "AladdinSDKLogDrop.h"

#define SDK_LOG_VERSION "0.0.5" // 库的版本

#define SDK_LOG_PROTOCOL_NAME "Aladdin" // 协议名称
#define SDK_LOG_PROTOCOL_VERSION "1.0.0" // 协议版本

#define AUTH_DONOT_SAVE // 保存配置到本地，用于调试

#define MAX_RETRY_TIMES 3 // 尝试或重发次数

#define MAX_CFG_UDPTIME 100   // 一次获取配置最大时长(ms数)
#define MAX_LOG_UDPTIME 30000 // 一次发送日志最大时长(ms数)

class AladdinSDKLogPack {
public:
    AladdinSDKLogPack(const char* module_name, const char* module_version, const char* sdk_name, const char* sdk_version);

    ~AladdinSDKLogPack();

    bool initial() { return initial_; } // 确认是否初始化过
    void version(char* ver) { strncpy(ver, SDK_LOG_VERSION, strlen(SDK_LOG_VERSION)); } // 获取版本信息，要求传入指针内存长度不小于16

    int logAuth(); // 云端配置是否允许记录日志：<0没收到配置信息，0-不允许，1-允许级别低，2-允许级别中，3-允许级别高，其他-未定义
    bool logSent(int log_type, const char* log_buf, int log_len, long log_time, const char* lob_str, bool (*log_cb)(int)); // 日志类型：0-图像，1-视频，2-音频，3-文本，其他-其他类型

private:
    const int head_size = 256; // 文件头
    const int max_data_size = 10 * 1024; // 数据区

    char* cfg_req_;
    char* cfg_res_;

    char* log_sent_;
    char* log_recv_;

    int cfg_auth_ = -1; // 配置是否允许记录日志

    const int max_buffer_size = 16 * 1024 * 1024; // 最大缓存尺寸
    const int max_string_size = 16; // 日志描述信息的最大尺寸

    int log_type_ = 0;
    char* log_buf_ = 0;
    int log_len_ = 0;
    long log_time_ = 0;
    char* log_str_ = 0;
    bool (*log_cb_)(int) = 0; // 回调函数

protected:
    void worker(); // 主要工作线程：持续监听UDP的config数据，收到发送数据启动发送

    bool decode(char* buf); // 对收到的数据进行包头解析
    bool encode(char* buf); // 对发送的数据进行包头打包

    inline int cfg_udp();
    inline int log_udp();

private:
    bool initial_ = false; // 是否完成初始化的标志
    std::atomic<int> status_;// 发送状态（-1表示空闲）

    int cfg_retry_count = 0; // 配置重试次数
    int log_retry_count = 0; // 日志重试次数

    std::shared_ptr<AladdinSDKLogCommUDP> comm_;
    std::shared_ptr<AladdinSDKLogDrop> drop_;

    std::string module_name_;
    std::string module_version_;

    std::string sdk_name_;
    std::string sdk_version_;

    std::condition_variable cond_;
    std::mutex mutex_;
    std::shared_ptr<std::thread> thread_;
};

#endif // __ALADDINSDK_LOG_PACK_H__

