#pragma once
#include "stdint.h"
#include "ShowerCode.h"

class IpdBuffer
{
public:

	void WriteByte(const uint8_t byte);
	ShowerCode Take(uint8_t* buf);
	uint8_t GetRequestSize();
	void Clear();

private:

	const static uint8_t BUF_SZ = 200;
    uint8_t _buf[BUF_SZ] = { };
    uint8_t _count = 0;
    uint8_t _head = 0;
    uint8_t _tail = 0;
};
