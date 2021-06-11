#pragma once
#include "stdint.h"

// Замеряет время на основе часов реального времени (RTC).
class RealTimeClockStopwatch
{
public:
	
	void Reset();
	uint32_t ElapsedSeconds();
	bool Timeout(uint16_t seconds);
	
private:
	
	volatile uint32_t _lastTime;
};

