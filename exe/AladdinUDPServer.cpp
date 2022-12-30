#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include <fstream>
#include <iostream> // 用于读写二进制文件
#include <unistd.h> // 否则调用close socket会报错
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define SOCKET_IP "127.0.0.1" // 暂时未用
#define SOCKET_CFG_PORT 8080
#define SOCKET_LOG_PORT 8081

#define SOCKET_TIMEOUT_SEC 10 // 超时的s数
#define SOCKET_TIMEOUT_USEC 0 // 超时的us数

#define PACK_HEAD_SIZE 256
#define PACK_DATA_SIZE 10*1024
#define MAX_FILE_SIZE  32*1024*1024 // 最大分配32M以备测试需要

static long getCurrentTimeStamp()
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return tp.tv_sec * 1000 + tp.tv_usec / 1000;
}

static void getBinFileName(char* filename)
{
    long ts = getCurrentTimeStamp();
    sprintf(filename, "%ld.bin", ts);
}

int init(int port, struct sockaddr_in* psockaddr)
{
    int sockfd;

    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        printf("init socket create failed!\n");
        return -1;
    }

    memset(psockaddr, 0, sizeof(struct sockaddr_in));
    psockaddr->sin_family = AF_INET;
    psockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    psockaddr->sin_port = htons(port);

    if (bind(sockfd, (struct sockaddr *)psockaddr, sizeof(sockaddr)) < 0) {
        close(sockfd);
        printf("init socket bind failed!\n");
        return -1;
    }

    printf("init socket port %d succeed!\n", port);
    return sockfd;
}

bool uninit(int sockfd)
{
    if (sockfd < 0) {
        printf("uninit socket close failed!\n");
        return false;
    }

    close(sockfd);

    printf("uninit socket close succeed!\n");
    return true;
}

struct pack_info {
    int pack_num_;
    int pack_index_;
    int pack_addr_;
    int pack_len_;
}packInfo;

struct log_info {
    std::string module_name_;
    std::string module_version_;
    std::string sdk_name_;
    std::string sdk_version_;

    std::string log_str_;
    long log_time_;
    int log_type_;
    int log_len_;
    char* log_buf_;
}logInfo;

bool parse(const char* pbuf, char* file_data) {
    /* module info */
    std::string module_name = std::string(pbuf + 32);
    std::string module_version = std::string(pbuf + 48);
    if (module_name.empty() || module_version.empty()) {
        printf("module info error! module name %s version %s\n", module_name.c_str(), module_version.c_str());
        return false;
    }

    /* sdk info */
    std::string sdk_name = std::string(pbuf + 64);
    std::string sdk_version = std::string(pbuf + 80);
    if (sdk_name.empty() || sdk_version.empty()) {
        printf("sdk info error! sdk name %s version %s\n", sdk_name.c_str(), sdk_version.c_str());
        return false;
    }

    /* log info */
    std::string log_str = std::string(pbuf + 128); // 注意传过来的字符串可能长度为16，没法有效截断；也可能为空
    long log_time = (long)(*((int64_t*)(pbuf + 144)));
    int log_type  =  (int)(*((int16_t*)(pbuf + 152)));
    int log_len   =  (int)(*((int32_t*)(pbuf + 154)));
    if (log_time <= 0 || (log_type < 0 || log_type > 4) || (log_len <= 0 || log_len > MAX_FILE_SIZE)) {
        printf("log info error! log_time %ld log_type %d log_len %d\n", log_time, log_type, log_len);
        return false;
    }

    /* package info */
    int pack_num  =  (int)(*((int32_t*)(pbuf + 160)));
    int pack_index=  (int)(*((int32_t*)(pbuf + 164)));
    int pack_addr =  (int)(*((int32_t*)(pbuf + 168)));
    int pack_len  =  (int)(*((int32_t*)(pbuf + 172)));
    if (pack_num <= 0 || (pack_index < 0 || pack_index >= pack_num) || (pack_addr < 0 || pack_addr >= log_len) || pack_len <= 0) {
        printf("pack info error! pack_num %d pack_index %d pack_addr %d pack_len %d\n", pack_num, pack_index, pack_addr, pack_len);
        return false;
    }

    if (pack_index == 0) {
        logInfo.module_name_ = module_name;
        logInfo.module_version_ = module_version;

        logInfo.sdk_name_ = sdk_name;
        logInfo.sdk_version_ = sdk_version;

        logInfo.log_str_  = log_str;
        logInfo.log_time_ = log_time;
        logInfo.log_type_ = log_type;
        logInfo.log_len_  = log_len;

        packInfo.pack_num_ = pack_num;
        packInfo.pack_index_ = pack_index;
        packInfo.pack_addr_ = pack_addr;
        packInfo.pack_len_ = pack_len;
    }
    else {
        if (logInfo.module_name_ != module_name || logInfo.module_version_ != module_version) {
            printf("module info missmatch! name (%s vs %s) version (%s vs %s)\n", logInfo.module_name_.c_str(), module_name.c_str(), logInfo.module_version_.c_str(), module_version.c_str());
            return false;
        }

        if (logInfo.sdk_name_ != sdk_name || logInfo.sdk_version_ != sdk_version) {
            printf("sdk info missmatch! name (%s vs %s) version (%s vs %s)\n", logInfo.sdk_name_.c_str(), sdk_name.c_str(), logInfo.sdk_version_.c_str(), sdk_version.c_str());
            return false;
        }

        if (logInfo.log_time_ != log_time || logInfo.log_type_ != log_type || logInfo.log_len_ != log_len) {
            printf("log info missmatch! log_time (%d vs %d) log_type (%d vs %d) log_len (%d vs %d)\n", logInfo.log_time_, log_time, logInfo.log_type_, log_type, logInfo.log_len_, log_len);
            return false;
        }

        if (packInfo.pack_num_ != pack_num) {
            printf("pack info missmatch! packnum (%d vs %d)\n", packInfo.pack_num_, pack_num);
            return false;
        }
    }

    printf("UDP package index %d (total %d packages), address from %d to %d (total %d bytes)\n", pack_index, pack_num, pack_addr, pack_addr + pack_len - 1, log_len); 
    memcpy(file_data + pack_addr, pbuf + PACK_HEAD_SIZE, pack_len);

    if (pack_index == pack_num - 1) { // 最后一个包就要存文件了
        std::string lot_type_str;

        switch(logInfo.log_type_) {
            case 0:
                  lot_type_str = "image";
                  break;
            case 1:
                  lot_type_str = "video";
                  break;
            case 2:
                  lot_type_str = "audio";
                  break;
            case 3:
                  lot_type_str = "texts";
                  break;
            default:
                  lot_type_str = "other";
        }

        std::string filename = logInfo.module_name_ + "_" + logInfo.module_version_ + "_" +
                               logInfo.sdk_name_ + "_" + logInfo.sdk_version_ + "_" +
                               logInfo.log_str_ + "_" + std::to_string(logInfo.log_time_) + "_" + lot_type_str + ".log";

        if (access(filename.c_str(), F_OK) != 0) { // access这个函数位于unistd.h中
            std::ofstream outfile(filename.c_str(), std::ios::out | std::ios::binary);
            outfile.write(file_data, log_len);
            outfile.flush();
            outfile.close();

            printf("UDP log saved at %s\n", filename.c_str()); // 要是文件不存在再存
        }
        else {
            printf("UDP log exist at %s\n", filename.c_str()); // 要是文件已存在跳过
        }
    }

    return true;
}

int main()
{
    int retvalue = -1;

    struct sockaddr_in sockaddr_cfg;
    struct sockaddr_in sockaddr_log;

    int socket_cfg = init(SOCKET_CFG_PORT, &sockaddr_cfg);
    int socket_log = init(SOCKET_LOG_PORT, &sockaddr_log);
    if (socket_cfg < 0 || socket_log < 0) {
        printf("socket init failed : cfg %d log %d\n", socket_cfg, socket_log);
        return retvalue;
    }

    int sockaddr_len = sizeof(sockaddr_in);

    char* buf_cfg = (char*)malloc(PACK_HEAD_SIZE * sizeof(char));
    char* buf_log = (char*)malloc((PACK_HEAD_SIZE + PACK_DATA_SIZE) * sizeof(char));
    char* file_data = (char*)malloc(MAX_FILE_SIZE * sizeof(char));
    if (buf_cfg == 0 || buf_log == 0 || file_data == 0) {
        printf("socket memory failed : head %d data %d file %d\n", PACK_HEAD_SIZE, PACK_DATA_SIZE, MAX_FILE_SIZE);
        goto hexit;
    }

    struct timeval tv; // 必须在循环内设置数值，否则只有第一次循环有效
    fd_set rd_fd;

    while (true) {
        FD_ZERO(&rd_fd);
        FD_SET(socket_cfg, &rd_fd);
        FD_SET(socket_log, &rd_fd);

        tv.tv_sec = SOCKET_TIMEOUT_SEC;
        tv.tv_usec= SOCKET_TIMEOUT_USEC;

        int max_fd = socket_cfg > socket_log ? socket_cfg : socket_log;
        int ret = select(max_fd + 1, &rd_fd, NULL, NULL, &tv);
        if (ret == -1) {
            printf("socket select error\n");
            break;
        }
        else if (ret == 0) {
            printf("socket timeout occur\n");
            continue;
        }
        else {
            if (FD_ISSET(socket_cfg, &rd_fd)) {
                int recv_len = recvfrom(socket_cfg, buf_cfg, PACK_HEAD_SIZE, 0, (struct sockaddr *)&sockaddr_cfg, (socklen_t*)&sockaddr_len);
                if (recv_len <= 0) {
                    printf("socket cfg none %d\n", recv_len);
                }
                else if (recv_len > PACK_HEAD_SIZE) {
                    printf("socket cfg exceed %d\n", recv_len);
                }
                else if (std::string(buf_cfg + 112) != "cfgRequest") {
                    printf("socket cfg unknown %d\n", recv_len);
                }
                else {
                    printf("socket cfg request %d\n", recv_len);

                #if 1
                    sprintf(buf_cfg + 112, "%s", "cfgResponse");

                    sprintf(buf_cfg + 128,"%s", "true");
                    sprintf(buf_cfg + 160,"%s", "mid");

                    sendto(socket_cfg, buf_cfg, PACK_HEAD_SIZE, 0, (struct sockaddr *)&sockaddr_cfg, sockaddr_len);
                #else
                    char filename[64] = {0};
                    getBinFileName(filename);
                    std::ofstream outfile(filename, std::ios::out | std::ios::binary);
                    outfile.write(buf_cfg, recv_len);
                    outfile.flush();
                    outfile.close();
                #endif
                }
            }

            if (FD_ISSET(socket_log, &rd_fd)) {
                int recv_len = recvfrom(socket_log, buf_log, PACK_HEAD_SIZE + PACK_DATA_SIZE, 0, (struct sockaddr *)&sockaddr_log, (socklen_t*)&sockaddr_len);
                if (recv_len <= 0) {
                    printf("socket log none %d\n", recv_len);
                }
                else if (recv_len < PACK_HEAD_SIZE || recv_len > PACK_HEAD_SIZE + PACK_DATA_SIZE) {
                    printf("socket log exceed %d\n", recv_len);
                }
                else if (std::string(buf_log + 112) != "logSent") {
                    printf("socket cfg unknown %d\n", recv_len);
                }
                else {
                    printf("socket log received %d\n", recv_len);

                    if (!parse(buf_log, file_data)) { // 注意最后一个包会存文件，这会导致下面的应答不及时，客户端超时会重发一次！
                        printf("socket log error\n");
                        continue; // 解析错误不用应答
                    }

                #if 1
                    sprintf(buf_log + 112, "%s", "logReceive");

                    sendto(socket_log, buf_log, PACK_HEAD_SIZE, 0, (struct sockaddr *)&sockaddr_log, sockaddr_len);
                #else
                    char filename[64] = {0};
                    getBinFileName(filename);
                    std::ofstream outfile(filename, std::ios::out | std::ios::binary);
                    outfile.write(buf_log, recv_len);
                    outfile.flush();
                    outfile.close();
                #endif
                }
            }
        }
    }

    retvalue = 0;

hexit:
    uninit(socket_cfg);
    uninit(socket_log);

    if (buf_cfg) {
        free(buf_cfg);
        buf_cfg = 0;
    }
    if (buf_log) {
        free(buf_log);
        buf_log = 0;
    }

    if (file_data) {
        free(file_data);
        file_data = 0;
    }

    return retvalue;
}
