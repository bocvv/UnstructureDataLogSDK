#include "AladdinSDKLogPack.h"

#include "cus_log.h"

#ifndef AUTH_DONOT_SAVE
#include <fstream>
#include <iostream> // 用于读写二进制文件
#endif

static long getCurrentTimeStamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

AladdinSDKLogPack::AladdinSDKLogPack(const char* module_name, const char* module_version,
                                     const char* sdk_name, const char* sdk_version):
                                     module_name_(module_name), module_version_(module_version),
                                     sdk_name_(sdk_name), sdk_version_(sdk_version)
{
    comm_ = std::make_shared<AladdinSDKLogCommUDP>();
    drop_ = std::make_shared<AladdinSDKLogDrop>();

    cfg_req_ = new char[head_size];
    cfg_res_ = new char[head_size];

    log_sent_ = new char[head_size + max_data_size];
    log_recv_ = new char[head_size];

    log_buf_ = new char[max_buffer_size];
    log_str_ = new char[max_string_size];

    initial_ = true; // 先申请资源再启动线程
    status_  = -1;
    thread_ = std::make_shared<std::thread>(&AladdinSDKLogPack::worker, this);
}

AladdinSDKLogPack::~AladdinSDKLogPack()
{
    initial_ = false;// 先停止线程再释放资源
    this->thread_->join();

    if (cfg_req_) {
        delete[] cfg_req_;
        cfg_req_ = NULL;
    }
    if (cfg_res_) {
        delete[] cfg_res_;
        cfg_res_ = NULL;
    }
    if (log_sent_) {
        delete[] log_sent_;
        log_sent_ = NULL;
    }
    if (log_recv_) {
        delete[] log_recv_;
        log_recv_ = NULL;
    }

    if (log_buf_) {
        delete[] log_buf_;
        log_buf_ = NULL;
    }
    if (log_str_) {
        delete[] log_str_;
        log_str_ = NULL;
    }
}

int AladdinSDKLogPack::logAuth()
{
    int cfg_auth;

    LOGD("[AladdinSDKLogPack] logAuth start!");
    std::unique_lock<std::mutex> locker(mutex_);

    cfg_auth = cfg_auth_;

    locker.unlock();
    LOGD("[AladdinSDKLogPack] logAuth with %d", cfg_auth);

    return cfg_auth;
}

// 这个函数里面如果加锁，有点相互嵌套的情况导致一些异常情况：
// 例如测试发现一旦传入的buf/len数据量大，notify之后持续触发不了wait_for
// 情况分析首先是在worker中只有两种情况能获得互斥量：
// 1. while循环结束本次循环之后开始新循环之前，间隙较小，可以不考虑
// 2. wait_for等待之时，时间较长，出现概率大
// 猜测原因可能是notify之后wait_for要触发需要获得互斥量，当获得互斥量时notify信号已经失效了
// 临时解决方案是利用原子变量(iambusy)来确保数据拷贝的时间错开。屏蔽了锁后，实测现象就消失了
// 有个问题是如果此时正在获取配置信息，可能会错过一次日志发送（如果有锁的话会等待)，可以考虑cfg和log用两个独立线程来做
bool AladdinSDKLogPack::logSent(int log_type, const char* log_buf, int log_len, long log_time, const char* log_str, bool (*log_cb)(int))
{
    if (((log_type >= 0 && log_type <= 4) && log_buf && (log_len > 0 && log_len <= max_buffer_size)) == false)
        return false;

    if (status_ >= 0)
        return false; // 要是当前还在发送中就不能继续发送

    LOGD("[AladdinSDKLogPack] logSent start!");
    //std::unique_lock<std::mutex> locker(mutex_);

    log_type_ = log_type;
    memcpy(log_buf_, log_buf, log_len);
    log_len_ = log_len;
    log_time_ = log_time;
    memset(log_str_, 0x00, max_string_size); // 要是用户没有传入相关信息，那信息将为空
    if (log_str && strlen(log_str))
        memcpy(log_str_, log_str, strlen(log_str) > max_string_size ? max_string_size : strlen(log_str));
    log_cb_ = log_cb;

    //locker.unlock();
    LOGD("[AladdinSDKLogPack] logSent with type %d buf %x len %d time %ld string %s callback %x", log_type_, log_buf_, log_len_, log_time_, log_str_, log_cb_);

    cond_.notify_one();

    return true;
}

bool AladdinSDKLogPack::decode(char* buf)
{
    if (std::string(buf + 0) != SDK_LOG_PROTOCOL_NAME)
        return false;

    if (std::string(buf + 32) != module_name_ &&
        std::string(buf + 64) != sdk_name_) // 可以执行module级别策略也可以sdk级别策略
        return false;

    return true;
}

bool AladdinSDKLogPack::encode(char* buf)
{
    sprintf(buf + 0,  "%s", SDK_LOG_PROTOCOL_NAME);
    sprintf(buf + 16, "%s", SDK_LOG_PROTOCOL_VERSION);

    sprintf(buf + 32, "%s", module_name_.c_str());
    sprintf(buf + 48, "%s", module_version_.c_str());
    sprintf(buf + 64, "%s", sdk_name_.c_str());
    sprintf(buf + 80, "%s", sdk_version_.c_str());

    return true;
}

inline int AladdinSDKLogPack::cfg_udp()
{
    int ret = -1; // 返回值：-1-还没获取到，0-失败，1-成功

    long timestamp = getCurrentTimeStamp();

    sprintf(cfg_req_ + 96, "%s", std::to_string(timestamp).c_str());
    sprintf(cfg_req_ + 112, "%s", "cfgRequest");

    comm_->out(SOCKET_CFG_PORT, cfg_req_, head_size);

    while (ret < 0) { // 这里重复尝试是避免被一些其他人的配置干扰，为防止持续循环会有总时间限制的
        if (comm_->in(SOCKET_CFG_PORT, cfg_res_, head_size) == false) {
            LOGD("[AladdinSDKLogPack] config noreply!");
            ret = 0; // 没有应答失败
        }
        else {
            #ifndef AUTH_DONOT_SAVE
                char filename[16] = "./AladdinCfg.bin"; // 将收到的配置数据存储在这里用于debug
                std::ofstream outfile(filename, std::ios::out | std::ios::binary);
                outfile.write(cfg_res_, head_size);
                outfile.flush();
                outfile.close();
            #endif

            if (decode(cfg_res_) && std::string(cfg_res_ + 112) == "cfgResponse") {
                std::string logUpload = std::string(cfg_res_ + 128); // 是否允许记录日志
                std::string logLevel  = std::string(cfg_res_ + 160); // 日志记录级别设置

                if (logUpload != "true") {
                    cfg_auth_ = 0; // 不允许记录日志
                }
                else {
                    if (logLevel == "low")
                        cfg_auth_ = 1;
                    else if (logLevel == "mid")
                        cfg_auth_ = 2;
                    else if (logLevel == "high")
                        cfg_auth_ = 3;
                    else
                        cfg_auth_ = 1; // 默认的日志等级
                }

                LOGD("[AladdinSDKLogPack] config succeed!");
                ret = 1; // 获取配置成功
            }
            else {
                LOGD("[AladdinSDKLogPack] config ignored!");
                ret = -1; // 错误数据或者别人的配置
            }
        }

        if (getCurrentTimeStamp() - timestamp > MAX_CFG_UDPTIME) {
            LOGD("[AladdinSDKLogPack] config timeout!");
            break;
        }
    }

    return ret;
}

inline int AladdinSDKLogPack::log_udp()
{
    status_ = 0;
    if (log_cb_ && status_ >= 0) log_cb_(0); // 开始进入也执行一下回调通知发送开始

    long timestamp = getCurrentTimeStamp();

    sprintf(log_sent_ + 96, "%s", std::to_string(timestamp).c_str());
    sprintf(log_sent_ + 112, "%s", "logSent");

    /* info */  memcpy(log_sent_ + 128, log_str_, max_string_size);
    drop_->drop8BytesToBuffer(log_sent_ + 144, (int64_t)log_time_);
    drop_->drop2BytesToBuffer(log_sent_ + 152, (int16_t)log_type_);
    drop_->drop4BytesToBuffer(log_sent_ + 154, (int32_t)log_len_);

    /* packnum */
    int packNum = log_len_ / max_data_size;
    if (log_len_ % max_data_size != 0) packNum ++;

    log_retry_count = 0; // 发送之前先清零

    int baseIndex = -1; // 用于记录重发的位置
    for (int packIndex = 0; packIndex < packNum; packIndex ++) {
        int packAddr= packIndex * max_data_size; // 本数据包发送的起始地址

        int packLen = max_data_size; // 本数据包发送的长度
        if (log_len_ - packAddr < max_data_size)
            packLen = log_len_ - packIndex * max_data_size;

        drop_->drop4BytesToBuffer(log_sent_ + 160, (int32_t)packNum);
        drop_->drop4BytesToBuffer(log_sent_ + 164, (int32_t)packIndex);
        drop_->drop4BytesToBuffer(log_sent_ + 168, (int32_t)packAddr);
        drop_->drop4BytesToBuffer(log_sent_ + 172, (int32_t)packLen);

        memcpy(log_sent_ + 256, log_buf_ + packIndex * max_data_size, packLen);

        comm_->out(SOCKET_LOG_PORT, log_sent_, head_size + packLen);

        if (comm_->in(SOCKET_LOG_PORT, log_recv_, head_size) && decode(log_recv_) && std::string(log_recv_ + 112) == "logReceive") {
            log_retry_count = 0;
            LOGD("[AladdinSDKLogPack] logging succeed! between %d and %d", baseIndex + 1, packIndex);

            baseIndex = packIndex; // 记录一下成功发送后的序号
        }
        else {
            log_retry_count ++;
            LOGD("[AladdinSDKLogPack] logging retry %d times! between %d and %d", log_retry_count, baseIndex + 1, packIndex);

            packIndex = baseIndex; // 要是失败了就从上次成功序号之后位置重发（本轮循环结束packIndex会自增1的）

            if (log_retry_count >= MAX_RETRY_TIMES) {
                LOGD("[AladdinSDKLogPack] logging retried too much, exit this task!");
                break; // 也不能无限次重发啊，要是尝试多次依旧失败，停止发送放弃这个日志
            }
        }

        if (getCurrentTimeStamp() - timestamp > MAX_LOG_UDPTIME) {
            LOGD("[AladdinSDKLogPack] logging have been timeout, drop this task!");
            break; // 文件特别大并且系统资源紧张时可能特别耗时，防止长时间无响应限制总时长
        }

        status_ = (int)((baseIndex + 1.0f) / packNum * 100); // 没有用packIndex是因为它有重发数值并非递增的
        if (log_cb_) log_cb_(status_); // 执行回调
    }

    if (log_cb_ && status_ < 100) log_cb_(100); // 万一失败也执行以下回调通知发送完毕
    status_ = -1; // 恢复到初始状态

    return baseIndex == packNum - 1;
}

void AladdinSDKLogPack::worker()
{
    LOGD("[AladdinSDKLogPack] worker start!");

    memset(cfg_req_, 0x0, head_size);
    encode(cfg_req_);

    memset(log_sent_, 0x0, head_size + max_data_size);
    encode(log_sent_);

    while(initial_) {
        std::unique_lock<std::mutex> locker(mutex_); // 每次循环，首先尝试加锁

        if (cfg_auth_ < 0) cfg_udp(); // 要是还没获取到配置，优先获取一次，否则要等timeout才会执行一次

        if (cond_.wait_for(locker, std::chrono::seconds(1)) == std::cv_status::timeout) { // 获得锁之后等待条件变量触发，等待过程中解锁互斥量
            // 获取配置（这是timeout才会执行，既然没有日志要发就刷新一下配置）
            if (cfg_udp() < 0) {
                cfg_retry_count ++;
                LOGD("[AladdinSDKLogPack] config retry %d times!", cfg_retry_count);

                if (cfg_retry_count >= MAX_RETRY_TIMES) {
                    cfg_auth_ = -1; // 获取配置失败，将视为日志服务通信失败，等下要优先去获取
                    LOGD("[AladdinSDKLogPack] config retried too much, fail to comm!");
                }
            }
            else {
                cfg_retry_count = 0; // 条件放宽一些，(无论收到的结果是否对)这里直接清零
                LOGD("[AladdinSDKLogPack] config succeed!");
            }
        }
        else {
            // 触发发送
            long before_ms = getCurrentTimeStamp();
            LOGD("[AladdinSDKLogPack] logging todo! addr %x bytes %d type %d times %ld", log_buf_, log_len_, log_type_, log_time_);

            int ret = log_udp();

            long after_ms = getCurrentTimeStamp();
            LOGD("[AladdinSDKLogPack] logging done! %ldms %s", after_ms - before_ms, ret ? "succeed" : "failed");
        }
    }

    LOGD("[AladdinSDKLogPack] worker ended!");
}
