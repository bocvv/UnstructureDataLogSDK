#include "AladdinSDKLogDrop.h"


AladdinSDKLogDrop::AladdinSDKLogDrop()
{
}

AladdinSDKLogDrop::~AladdinSDKLogDrop()
{
}

void AladdinSDKLogDrop::dropByteToBuffer(char* pbuf, int8_t value)
{
    *pbuf = value;
}

void AladdinSDKLogDrop::drop2BytesToBuffer(char* pbuf, int16_t value)
{
    *(pbuf + 0) = value & 0xFF;        // low byte
    *(pbuf + 1) = (value >> 8) & 0xFF; // high byte
}

void AladdinSDKLogDrop::drop4BytesToBuffer(char* pbuf, int32_t value)
{
    *(pbuf + 0) = value & 0xFF;
    *(pbuf + 1) = (value >> 8) & 0xFF;
    *(pbuf + 2) = (value >> 16) & 0xFF;
    *(pbuf + 3) = (value >> 24) & 0xFF;
}

void AladdinSDKLogDrop::drop8BytesToBuffer(char* pbuf, int64_t value)
{
    *(pbuf + 0) = value & 0xFF;
    *(pbuf + 1) = (value >> 8) & 0xFF;
    *(pbuf + 2) = (value >> 16) & 0xFF;
    *(pbuf + 3) = (value >> 24) & 0xFF;
    *(pbuf + 4) = (value >> 32) & 0xFF;
    *(pbuf + 5) = (value >> 40) & 0xFF;
    *(pbuf + 6) = (value >> 48) & 0xFF;
    *(pbuf + 7) = (value >> 56) & 0xFF;
}

