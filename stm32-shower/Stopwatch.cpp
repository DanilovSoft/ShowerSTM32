#include "Stopwatch.h"
#include "stm32f10x.h"
#include "core_cm3.h"
#include "Common.h"

void Stopwatch::Initialize()
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;     // Разрешаем TRACE
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;     // Разрешаем счетчик тактов
    DWT->CYCCNT = 0;
}


void Stopwatch::Start()
{
	_startValue = DWT->CYCCNT;
}


void Stopwatch::Stop()
{
	_stopValue = DWT->CYCCNT;
}


uint32_t Stopwatch::GetElapsed()
{
	uint32_t elapsed = _stopValue - _startValue; // всегда верно, даже если _stopValue < _startValue
	return elapsed;
}
