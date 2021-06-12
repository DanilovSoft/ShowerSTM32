#pragma once
#include "IpdBuffer.h"

constexpr auto IPD_BUF_SZ = 5;

class ConnectionsBuffer final
{
public:
	
	IpdBuffer* operator[](const uint8_t index);
	void Clear(const uint8_t connection_id);
	void ClearAll();
	uint8_t GetRequestSize(uint8_t &connection_id);
	
private:
	
    IpdBuffer m_buf[IPD_BUF_SZ];
    uint8_t m_nextConnectionId = 0;
    void MoveNext();
};
