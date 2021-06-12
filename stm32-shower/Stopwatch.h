#pragma once
#include "FreeRTOS.h"
#include "task.h"
#include "Common.h"

// Измеряет время в миллисекундах на основе тиков FreeFTOS.
class Stopwatch final
{
public:
	
	Stopwatch()
	{
		Reset();
	}
	
	// Сбрасывает отсчёт времени.
	void Reset()
	{
		m_beginTickCount = xTaskGetTickCount();
	}
	
	bool TimedOut(uint32_t msec)
	{
		return GetElapsedMsec() > msec;
	}
	
	// Возвращает число квантов времени FreeRTOS которое прошло с момента Reset.
	uint32_t GetElapsedTicks()
	{
		uint32_t cur_tick_count = xTaskGetTickCount();
		return Common::abs(cur_tick_count, m_beginTickCount);
	}
    
	// Возвращает сколько прошло миллисекунд с момента Reset.
	uint32_t GetElapsedMsec()
	{
		uint32_t cur_tick_count = xTaskGetTickCount();
		uint32_t elapsed_msec = Common::abs(cur_tick_count, m_beginTickCount) * portTICK_PERIOD_MS;
		return elapsed_msec;
	}
	
private:
	
	volatile uint32_t m_beginTickCount;
};