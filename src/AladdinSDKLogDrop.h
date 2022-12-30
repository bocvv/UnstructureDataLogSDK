#ifndef __ALADDINSDK_LOG_DROP_H__
#define __ALADDINSDK_LOG_DROP_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

class AladdinSDKLogDrop {
public:
    AladdinSDKLogDrop();
    ~AladdinSDKLogDrop();

    void dropByteToBuffer(char* pbuf, int8_t value);
    void drop2BytesToBuffer(char* pbuf, int16_t value);
    void drop4BytesToBuffer(char* pbuf, int32_t value);
    void drop8BytesToBuffer(char* pbuf, int64_t value);
};

#endif // __ALADDINSDK_LOG_DROP_H__

