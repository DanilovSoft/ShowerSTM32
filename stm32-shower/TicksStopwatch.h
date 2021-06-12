#pragma once
#include "stdint.h"

// Замеряет время в тактах процессора.
class TicksStopwatch final
{
public:
	
	TicksStopwatch();
	static void Initialize();
	void Reset();
	void Stop();
	uint32_t GetElapsedTicks();
	
private:
	
    uint32_t m_startValue = 0;
    uint32_t m_stopValue = 0;
};

