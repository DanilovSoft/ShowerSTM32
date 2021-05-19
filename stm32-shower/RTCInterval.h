#pragma once
#include "stdint.h"

class RTCInterval
{
	volatile uint32_t _lastTime;
	
public:
	void Reset();
	uint32_t Elapsed();
	bool Timeout(uint16_t seconds);
};

