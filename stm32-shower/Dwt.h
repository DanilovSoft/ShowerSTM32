#pragma once
#include "stm32f10x.h"

class Dwt
{
	
public:

	void Init()
	{
		CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;     // Разрешаем TRACE
		DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;     // Разрешаем счетчик тактов
		DWT->CYCCNT = 0;
	}

	inline unsigned int Get() __attribute__((always_inline))
	{
		return DWT->CYCCNT;
	}

	inline unsigned int Compare(unsigned int tickCount) __attribute__((always_inline))
	{
		unsigned int stop = DWT->CYCCNT;
		if (stop > tickCount)
			return stop - tickCount;
		else
			return tickCount - stop;
	}

    inline float CompareMs(unsigned int tickCount) __attribute__((always_inline))
	{
		unsigned int stop = DWT->CYCCNT;
		unsigned int elapsed;
		if (stop > tickCount)
			elapsed = stop - tickCount;
		else
			elapsed = tickCount - stop;

		float msec = elapsed / (float)(SystemCoreClock / 1000.0);
		return msec;
	}

	inline uint32_t CompareUs(unsigned int tickCount) __attribute__((always_inline))
	{
		unsigned int stop = DWT->CYCCNT;
		unsigned int elapsed;
    	
		if (stop > tickCount)
			elapsed = stop - tickCount;
		else
			elapsed = tickCount - stop;

		uint32_t us = elapsed / (SystemCoreClock / 1000000);
		return us;
	}
};

extern Dwt g_dwt;