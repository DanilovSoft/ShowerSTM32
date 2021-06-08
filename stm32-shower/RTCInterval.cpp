#include "RTCInterval.h"
#include "stm32f10x_rtc.h"
#include "Common.h"

void RTCInterval::Reset()
{
    _lastTime = RTC_GetCounter();
}

uint32_t RTCInterval::Elapsed()
{
    return abs(RTC_GetCounter(), _lastTime);
}

bool RTCInterval::Timeout(uint16_t seconds)
{
    uint32_t elapsedSec = abs(RTC_GetCounter(), _lastTime);
    return elapsedSec > seconds;
}
