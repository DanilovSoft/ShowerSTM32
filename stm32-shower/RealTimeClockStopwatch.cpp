#include "RealTimeClockStopwatch.h"
#include "stm32f10x_rtc.h"
#include "Common.h"

void RealTimeClockStopwatch::Reset()
{
    _lastTime = RTC_GetCounter();
}

uint32_t RealTimeClockStopwatch::ElapsedSeconds()
{
    return abs(RTC_GetCounter(), _lastTime);
}

bool RealTimeClockStopwatch::Timeout(uint16_t seconds)
{
    uint32_t elapsedSec = abs(RTC_GetCounter(), _lastTime);
    return elapsedSec > seconds;
}
