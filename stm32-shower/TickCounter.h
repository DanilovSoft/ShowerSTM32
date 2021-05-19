#pragma once
#include "stm32f10x_rcc.h"
#include "Common.h"
#include "FreeRTOS.h"
#include "task.h"

// —читает количество тиков FreeFTOS
class TickCounter
{
	volatile uint32_t _lastValue;
	
public:
    TickCounter()
    {
        Restart();
    }
	
	void Restart()
	{
		_lastValue = xTaskGetTickCount();
	}
	
	bool TimeOut(uint32_t ms)
	{
		uint32_t curTime = xTaskGetTickCount();
		uint32_t elapsedMs = abs(curTime, _lastValue) * portTICK_PERIOD_MS;
		return elapsedMs > ms;
	}
	
    uint32_t GetElapsedTicks()
    {
        uint32_t curTime = xTaskGetTickCount();
        return  abs(curTime, _lastValue);
    }
    
	uint32_t GetElapsedMs()
	{
		uint32_t curTime = xTaskGetTickCount();
		uint32_t elapsedMs = abs(curTime, _lastValue) * portTICK_PERIOD_MS;
		return elapsedMs;
	}
};