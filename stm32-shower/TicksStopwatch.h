#pragma once
#include "stdint.h"

// Замеряет время в тактах процессора.
class TicksStopwatch
{
public:
	
	TicksStopwatch();
	static void Initialize();
	void Reset();
	void Stop();
	uint32_t GetElapsedTicks();
	
private:
	
    uint32_t _startValue = 0;
    uint32_t _stopValue = 0;
};

