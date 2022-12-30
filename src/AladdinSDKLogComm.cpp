#include "AladdinSDKLogComm.h"

#include "cus_log.h"

AladdinSDKLogCommUDP::AladdinSDKLogCommUDP()
{
    init();
}

AladdinSDKLogCommUDP::~AladdinSDKLogCommUDP()
{
    release();
}

bool AladdinSDKLogCommUDP::init()
{
    // Creating socket file descriptor
    if ((sockfd_cfg_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGD("[AladdinSDKLogCommUDP] create failed!");
        return false;
    }

    // Filling server information
    memset(&sockaddr_cfg_, 0, sizeof(struct sockaddr_in));
    sockaddr_cfg_.sin_family = AF_INET;
    sockaddr_cfg_.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr_cfg_.sin_port = htons(SOCKET_CFG_PORT);

  #if 0 // 因为是客户端无需bind和设置监听超时
    // Timeout setting
    struct timeval tv;
    tv.tv_sec = SOCKET_TIMEOUT_SEC;
    tv.tv_usec= SOCKET_TIMEOUT_USEC;
    setsockopt(sockfd_cfg_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(struct timeval));

    // Bind server
    if (bind(sockfd_cfg_, (struct sockaddr *)&sockaddr_cfg_, sizeof(sockaddr_cfg_)) < 0) {
        LOGD("[AladdinSDKLogCommUDP] bind failed!");
        close(sockfd_cfg_);
        return false;
    }
  #endif

    // -------------------------------------------------------

    // Creating socket file descriptor
    if ((sockfd_log_ = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        LOGD("[AladdinSDKLogCommUDP] create failed!");
        close(sockfd_cfg_);
        return false;
    }

    // Filling server information
    memset(&sockaddr_log_, 0, sizeof(struct sockaddr_in));
    sockaddr_log_.sin_family = AF_INET;
    sockaddr_log_.sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr_log_.sin_port = htons(SOCKET_LOG_PORT);
	
  #if 0 // 我们使用的是localhost且设置INADDR_ANY说明任意都可以
    // get hostname
    struct hostent* he = NULL;
    if ((he = gethostbyname(SOCKET_IP)) == NULL)
    {
        LOGD("[AladdinSDKLogCommUDP] get failed!");
        close(sockfd_cfg_);
        close(sockfd_log_);
    }
    sockaddr_log_.sin_addr= *((struct in_addr *)he->h_addr);
  #endif

    LOGD("[AladdinSDKLogCommUDP] init ok!");
    return true;
}

bool AladdinSDKLogCommUDP::release()
{
    if (sockfd_cfg_) {
        close(sockfd_cfg_);
        sockfd_cfg_ = -1;
    }
    if (sockfd_log_) {
        close(sockfd_log_);
        sockfd_log_ = -1;
    }
	
    LOGD("[AladdinSDKLogCommUDP] release ok!");
    return true;
}

bool AladdinSDKLogCommUDP::in(int port, char* buf, int len)
{
    if ((port != SOCKET_CFG_PORT && port != SOCKET_LOG_PORT) || (buf == NULL || len < 0)) {
        LOGD("[AladdinSDKLogCommUDP] port%d in failed", port);
        return false;
    }

    int sockfd_ = 0;
    struct sockaddr* sockaddr_ = 0;
    int sockaddr_len_ = 0;
    if (port == SOCKET_CFG_PORT) {
        sockfd_ = sockfd_cfg_;
        sockaddr_ = (struct sockaddr *)&sockaddr_cfg_;
        sockaddr_len_ = sizeof(sockaddr_cfg_);
    } else {
        sockfd_ = sockfd_log_;
        sockaddr_ = (struct sockaddr *)&sockaddr_log_;
        sockaddr_len_ = sizeof(sockaddr_log_);
    }

    fd_set rfds;
    FD_ZERO(&rfds);
    FD_SET(sockfd_, &rfds);

    struct timeval tv;
    tv.tv_sec = SOCKET_TIMEOUT_SEC;
    tv.tv_usec= SOCKET_TIMEOUT_USEC;

    int maxfd = sockfd_ + 1;
    if (select(maxfd, &rfds, NULL, NULL, &tv) <= 0) { // 如果返回0表示超时，-1表示错误
        LOGD("[AladdinSDKLogCommUDP] port%d in error/timeout", port);
        return false;
    }

    if (FD_ISSET(sockfd_, &rfds)) {
        if (recvfrom(sockfd_, (void*)buf, len, 0, // 成功的话返回接收的字节数，失败返回-1
                     sockaddr_, (socklen_t*)&sockaddr_len_) <= 0) { // 注意最后那个参数是个指针
            LOGD("[AladdinSDKLogCommUDP] port%d in none", port);
            return false;
        }
    }

    LOGD("[AladdinSDKLogCommUDP] port%d in done", port);
    return true;
}

bool AladdinSDKLogCommUDP::out(int port, const char* buf, int len)
{
    if ((port != SOCKET_CFG_PORT && port != SOCKET_LOG_PORT) || (buf == NULL || len < 0)) {
        LOGD("[AladdinSDKLogCommUDP] port%d out failed", port);
        return false;
    }

    int sockfd_ = 0;
    struct sockaddr* sockaddr_ = 0;
    int sockaddr_len_ = 0;
    if (port == SOCKET_CFG_PORT) {
        sockfd_ = sockfd_cfg_;
        sockaddr_ = (struct sockaddr *)&sockaddr_cfg_;
        sockaddr_len_ = sizeof(sockaddr_cfg_);
    } else {
        sockfd_ = sockfd_log_;
        sockaddr_ = (struct sockaddr *)&sockaddr_log_;
        sockaddr_len_ = sizeof(sockaddr_log_);
    }

    if (sendto(sockfd_, buf, len, 0, // 成功的话返回发送的字节数，失败返回-1
               sockaddr_, sockaddr_len_) <= 0) {
        LOGD("[AladdinSDKLogCommUDP] port%d out none", port);
        return false;
    }

    LOGD("[AladdinSDKLogCommUDP] port%d out done", port);
    return true;
}

