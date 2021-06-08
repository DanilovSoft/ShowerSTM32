#pragma once
#include "stdint.h"

// Замеряет время на основе часов реального времени (RTC).
class RTCInterval
{
private:
	
	volatile uint32_t _lastTime;
	
public:
	void Reset();
	uint32_t Elapsed();
	bool Timeout(uint16_t seconds);
};

