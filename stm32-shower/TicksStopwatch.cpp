#include "TicksStopwatch.h"
#include "stm32f10x.h"

void TicksStopwatch::Initialize()
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;     // Разрешаем TRACE.
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;     // Разрешаем счетчик тактов.
    DWT->CYCCNT = 0;
}

TicksStopwatch::TicksStopwatch()
{
	Reset();
}

void TicksStopwatch::Reset()
{
	_startValue = DWT->CYCCNT;
}

void TicksStopwatch::Stop()
{
	_stopValue = DWT->CYCCNT;
}

uint32_t TicksStopwatch::GetElapsedTicks()
{
	uint32_t elapsed = _stopValue - _startValue; // всегда верно, даже если _stopValue < _startValue
	return elapsed;
}
