#ifndef __ALADDINSDK_LOG_COMM_H__
#define __ALADDINSDK_LOG_COMM_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <unistd.h> // 否则调用close socket会报错
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#define SOCKET_IP "127.0.0.1"
#define SOCKET_CFG_PORT 8080
#define SOCKET_LOG_PORT 8081

#define SOCKET_TIMEOUT_SEC 0 // 超时的s数
#define SOCKET_TIMEOUT_USEC 100000 // 超时的us数

class AladdinSDKLogCommUDP {
public:
    AladdinSDKLogCommUDP();
    ~AladdinSDKLogCommUDP();

    bool in(int port, char* buf, int len); // 从port端口接收
    bool out(int port, const char* buf, int len); // 从port端口发送

protected:
    bool init();
    bool release();

private:
    int sockfd_cfg_ = -1;
    struct sockaddr_in sockaddr_cfg_;

    int sockfd_log_ = -1;
    struct sockaddr_in sockaddr_log_;
};

#endif // __ALADDINSDK_LOG_COMM_H__

