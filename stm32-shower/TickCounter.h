#pragma once
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"

// Измеряет время в миллисекундах на основе тиков FreeFTOS.
class TickCounter
{
public:
	
	TickCounter()
	{
		Reset();
	}
	
	// Начинает отсчёт времени сначала.
	void Reset()
	{
		_initialTickCount = xTaskGetTickCount();
	}
	
	bool TimedOut(uint32_t msec)
	{
		return GetElapsedMsec() > msec;
	}
	
	uint32_t GetElapsedTicks()
	{
		uint32_t tickCount = xTaskGetTickCount();
		return abs(tickCount, _initialTickCount);
	}
    
	// Возвращает сколько прошло миллисекунд с момента старта.
	uint32_t GetElapsedMsec()
	{
		uint32_t curTime = xTaskGetTickCount();
		uint32_t elapsedMs = abs(curTime, _initialTickCount) * portTICK_PERIOD_MS;
		return elapsedMs;
	}
	
private:
	
	volatile uint32_t _initialTickCount;
};