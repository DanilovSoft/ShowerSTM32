#pragma once
#include "IpdBuffer.h"

class ConnectionsBuffer final
{
public:
	
	IpdBuffer* operator[](const uint8_t index);
	void Clear(const uint8_t connection_id);
	void ClearAll();
	uint8_t GetRequestSize(uint8_t &connection_id);
	
private:
	static constexpr auto kIpdBufSize = 5;
    IpdBuffer m_buf[kIpdBufSize];
    uint8_t m_nextConnectionId = 0;
    void MoveNext();
};
