#pragma once
#include "stdint.h"
#include "stm32f10x.h"

// Замеряет время в тактах процессора.
class TicksStopwatch final
{
public:
	
	static void Initialize()
	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;     // Разрешаем TRACE.
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;      // Разрешаем счетчик тактов.
		DWT->CYCCNT = 0;
	}
	
	TicksStopwatch()
	{
		Reset();
	}

	void Reset()
	{
		m_startValue = DWT->CYCCNT;
	}

	void Stop()
	{
		m_stopValue = DWT->CYCCNT;
	}

	uint32_t GetElapsedTicks()
	{
		uint32_t elapsed = m_stopValue - m_startValue;  // Всегда верно, даже если _stopValue < _startValue.
		return elapsed;
	}
	
private:
	
    uint32_t m_startValue = 0;
    uint32_t m_stopValue = 0;
};

