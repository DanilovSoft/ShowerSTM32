#pragma once
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"

// Считает количество тиков FreeFTOS.
class TickCounter
{
private:
	
	volatile uint32_t _lastValue;
	
public:
	
    TickCounter()
    {
        Reset();
    }
	
	// Начинает отсчёт времени сначала.
	void Reset()
	{
		_lastValue = xTaskGetTickCount();
	}
	
	bool TimedOut(uint32_t msec)
	{
		return GetElapsedMsec() > msec;
	}
	
    uint32_t GetElapsedTicks()
    {
        uint32_t curTime = xTaskGetTickCount();
        return  abs(curTime, _lastValue);
    }
    
	// Возвращает сколько прошло миллисекунд с момента старта.
	uint32_t GetElapsedMsec()
	{
		uint32_t curTime = xTaskGetTickCount();
		uint32_t elapsedMs = abs(curTime, _lastValue) * portTICK_PERIOD_MS;
		return elapsedMs;
	}
};