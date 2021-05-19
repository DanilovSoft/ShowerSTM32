#pragma once
#include "IpdBuffer.h"

#define IPD_BUF_SZ      (5)

class ConnectionsBuffer
{
private:
    IpdBuffer _buf[IPD_BUF_SZ];
    uint8_t _nextConnectionId = 0;
    void MoveNext();
public:
    IpdBuffer* operator[](const uint8_t index);
    void Clear(const uint8_t connection_id);
    void ClearAll();
	uint8_t GetRequestSize(uint8_t &connection_id);
};
