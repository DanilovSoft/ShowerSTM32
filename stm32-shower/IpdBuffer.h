#pragma once
#include "stdint.h"
#include "ShowerCode.h"

class IpdBuffer final
{
public:

	void WriteByte(const uint8_t byte);
	ShowerCode Take(uint8_t* buf);
	uint8_t GetRequestSize();
	void Clear();

private:

	static const uint8_t kBufSize = 200;
    uint8_t m_buf[kBufSize] = { };
    uint8_t m_count = 0;
    uint8_t m_head = 0;
    uint8_t m_tail = 0;
};
